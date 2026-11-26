#include "httplib.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "AllocationManager.h"
#include "Common.h"
#include "FragmentationAnalyzer.h"
#include "FreeSegment.h"
#include "StorageManager.h"
#include "WorkloadSimulator.h"

static const int MAX_FILES = 5000;          // file ids 0..4999 (A-Z are 0..25)
static const Strategy ALL[3] = {Strategy::FIRST_FIT, Strategy::BEST_FIT, Strategy::WORST_FIT};

// ---------- tiny JSON helpers (we build JSON text by hand, no library needed) ----------
static std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}
static std::string num(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4) << v;
    return ss.str();
}
static std::string labelOf(int id) {
    return id < 26 ? std::string(1, (char)('A' + id)) : std::to_string(id);
}
static const char* strategyKey(Strategy s) {
    return s == Strategy::FIRST_FIT ? "first" : (s == Strategy::BEST_FIT ? "best" : "worst");
}
static bool parseStrategyKey(const std::string& k, Strategy& out) {
    if (k == "first") { out = Strategy::FIRST_FIT; return true; }
    if (k == "best")  { out = Strategy::BEST_FIT;  return true; }
    if (k == "worst") { out = Strategy::WORST_FIT; return true; }
    return false;
}

// ---------- the one simulator the page controls ----------
struct LogEntry { std::string text, result; bool ok; };

struct LastAction {
    std::string type = "none";    // allocate | release | init | random | none
    int id = -1, start = -1, length = 0;
    bool ok = true;
    std::string result, message;
};

struct Sim {
    int total = 40;
    Strategy strategy = Strategy::FIRST_FIT;
    std::unique_ptr<StorageManager> st;
    std::unique_ptr<FreeList> fl;
    std::unique_ptr<AllocationManager> am;
    FragmentationAnalyzer an;
    std::vector<LogEntry> log;
    int nextAutoId = 26;               // random fills use ids 26, 27, ... (shown as numbers)
    LastAction last;
    std::vector<double> history;       // fragmentation after every operation (for the Statistics page chart)

    void reset(int blocks, Strategy s) {
        total = blocks;
        strategy = s;
        st.reset(new StorageManager(total, MAX_FILES));     // M1's array
        fl.reset(new FreeList(total));                       // M2's linked list: one big free segment
        am.reset(new AllocationManager(*st, *fl, strategy)); // M2's allocator
        an = FragmentationAnalyzer();                        // M3's counters
        log.clear();
        nextAutoId = 26;
        history.clear();
        last = LastAction();
        last.type = "init";
        last.message = "Storage reset: " + std::to_string(total) + " free blocks";
    }

    AllocResult doAllocate(int id, int n) {
        AllocResult r = am->allocate(id, n);
        an.record(r);
        last = LastAction();
        last.type = "allocate"; last.id = id; last.length = n;
        last.ok = (r == AllocResult::SUCCESS);
        last.result = resultName(r);
        int s, l;
        if (last.ok && st->getFile(id, s, l)) last.start = s;
        std::ostringstream m;
        m << "ALLOCATE " << labelOf(id) << " (" << n << " blocks)";
        if (last.ok) m << " at block " << last.start;
        last.message = m.str();
        log.push_back({m.str(), last.result, last.ok});
        history.push_back(FragmentationAnalyzer::fragmentationOf(*fl));
        return r;
    }

    bool doRelease(int id) {
        int s = -1, l = 0;
        bool had = st->getFile(id, s, l);
        bool ok = am->release(id);
        if (!ok) an.recordBadRelease();
        last = LastAction();
        last.type = "release"; last.id = id; last.ok = ok;
        if (had) { last.start = s; last.length = l; }
        last.result = ok ? "RELEASED" : "IGNORED (file not allocated)";
        last.message = "RELEASE " + labelOf(id);
        log.push_back({last.message, last.result, ok});
        history.push_back(FragmentationAnalyzer::fragmentationOf(*fl));
        return ok;
    }
};

static Sim sim;
static std::mutex mtx;      // the server can handle several requests at once; this keeps them in order

static std::string stateJson() {
    Stats s = FragmentationAnalyzer::compute(*sim.st, *sim.fl);
    bool consistent = FragmentationAnalyzer::checkConsistency(*sim.st, *sim.fl);
    std::ostringstream o;
    o << "{\"total\":" << sim.total << ",\"strategy\":\"" << strategyKey(sim.strategy) << "\"";
    o << ",\"strategyName\":\"" << strategyName(sim.strategy) << "\"";

    o << ",\"blocks\":[";                                   // the ARRAY: file id per block, -1 = free
    for (int i = 0; i < sim.total; i++) { if (i) o << ","; o << (sim.st->at(i).occupied ? sim.st->at(i).fileID : -1); }
    o << "]";

    o << ",\"files\":[";                                    // the file table
    bool first = true;
    for (int id = 0; id < MAX_FILES; id++) {
        int st0, len;
        if (sim.st->getFile(id, st0, len)) {
            if (!first) o << ",";
            first = false;
            o << "{\"id\":" << id << ",\"label\":\"" << labelOf(id) << "\",\"start\":" << st0 << ",\"length\":" << len << "}";
        }
    }
    o << "]";

    o << ",\"free\":[";                                     // the LINKED LIST, node by node
    first = true;
    for (FreeSegment* p = sim.fl->getHead(); p; p = p->next) {
        if (!first) o << ",";
        first = false;
        o << "{\"start\":" << p->start << ",\"size\":" << p->size << "}";
    }
    o << "]";

    o << ",\"stats\":{\"total\":" << s.totalBlocks << ",\"occupied\":" << s.occupiedBlocks
      << ",\"free\":" << s.freeBlocks << ",\"segments\":" << s.freeSegments
      << ",\"largest\":" << s.largestSegment << ",\"fragmentation\":" << num(s.fragmentation) << "}";
    o << ",\"counters\":{\"success\":" << sim.an.successCount() << ",\"failFrag\":" << sim.an.fragmentationFailures()
      << ",\"failNoSpace\":" << sim.an.noSpaceFailures() << ",\"rejected\":" << sim.an.rejectedCount()
      << ",\"badReleases\":" << sim.an.badReleaseCount() << "}";
    o << ",\"consistent\":" << (consistent ? "true" : "false");

    o << ",\"action\":{\"type\":\"" << sim.last.type << "\",\"id\":" << sim.last.id << ",\"start\":" << sim.last.start
      << ",\"length\":" << sim.last.length << ",\"ok\":" << (sim.last.ok ? "true" : "false")
      << ",\"result\":\"" << esc(sim.last.result) << "\",\"message\":\"" << esc(sim.last.message) << "\"}";

    o << ",\"log\":[";                                      // last 60 log lines
    int from = (int)sim.log.size() > 60 ? (int)sim.log.size() - 60 : 0;
    for (int i = from; i < (int)sim.log.size(); i++) {
        if (i > from) o << ",";
        o << "{\"text\":\"" << esc(sim.log[i].text) << "\",\"result\":\"" << esc(sim.log[i].result)
          << "\",\"ok\":" << (sim.log[i].ok ? "true" : "false") << "}";
    }
    o << "]";

    o << ",\"history\":[";                                  // fragmentation after each operation (last 120)
    int hf = (int)sim.history.size() > 120 ? (int)sim.history.size() - 120 : 0;
    for (int i = hf; i < (int)sim.history.size(); i++) { if (i > hf) o << ","; o << num(sim.history[i]); }
    o << "],\"opCount\":" << sim.history.size() << "}";
    return o.str();
}

static std::string errorJson(const std::string& msg) {
    return "{\"error\":\"" + esc(msg) + "\"}";
}

static int toInt(const std::string& s, int fallback) {
    if (s.empty()) return fallback;
    for (size_t i = 0; i < s.size(); i++)
        if (!(std::isdigit((unsigned char)s[i]) || (i == 0 && s[i] == '-' && s.size() > 1))) return fallback;
    if (s.size() > 9) return fallback;
    return std::stoi(s);
}

static void json(httplib::Response& res, const std::string& body, int status = 200) {
    res.status = status;
    res.set_content(body, "application/json");
}

// one row of results for the comparison tables
static std::string rowJson(const RunResult& r) {
    std::ostringstream o;
    o << "{\"strategy\":\"" << strategyKey(r.strategy) << "\",\"name\":\"" << strategyName(r.strategy) << "\""
      << ",\"success\":" << r.success << ",\"failFrag\":" << r.failFrag << ",\"failNoSpace\":" << r.failNoSpace
      << ",\"badReleases\":" << r.badReleases << ",\"freeBlocks\":" << r.freeBlocks
      << ",\"freeSegments\":" << r.freeSegments << ",\"largest\":" << r.largest
      << ",\"finalFrag\":" << num(r.finalFrag) << ",\"avgFrag\":" << num(r.avgFrag)
      << ",\"timeUs\":" << num(r.timeUs) << ",\"consistent\":" << (r.consistent ? "true" : "false") << "}";
    return o.str();
}

int main(int argc, char* argv[]) {
 const char* env_port = std::getenv("PORT");

int port = argc > 1
    ? std::atoi(argv[1])
    : (env_port ? std::atoi(env_port) : 8080);
    sim.reset(40, Strategy::FIRST_FIT);

    httplib::Server svr;
    if (!svr.set_mount_point("/", "./web")) {
        std::cerr << "ERROR: cannot find the 'web' folder.\n"
                  << "Run this program from the project folder (the one that contains 'web', 'src' and 'include').\n";
        return 1;
    }
    svr.set_default_headers({{"Cache-Control", "no-store"}});   // always show the newest page while you edit

    svr.Get("/api/state", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        json(res, stateJson());
    });

    svr.Post("/api/init", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        int blocks = toInt(req.get_param_value("blocks"), 40);
        Strategy s = sim.strategy;
        parseStrategyKey(req.get_param_value("strategy"), s);
        if (blocks < 5 || blocks > 400) return json(res, errorJson("Blocks must be between 5 and 400 for the visual view."), 400);
        sim.reset(blocks, s);
        json(res, stateJson());
    });

    svr.Post("/api/strategy", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        Strategy s;
        if (!parseStrategyKey(req.get_param_value("name"), s)) return json(res, errorJson("Unknown strategy"), 400);
        sim.strategy = s;
        sim.am->setStrategy(s);
        sim.last = LastAction();
        sim.last.type = "strategy";
        sim.last.message = std::string("Strategy changed to ") + strategyName(s);
        sim.log.push_back({sim.last.message, "OK", true});
        json(res, stateJson());
    });

    svr.Post("/api/allocate", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        int id = WorkloadSimulator::parseFileId(req.get_param_value("id"));
        int n = toInt(req.get_param_value("size"), 0);
        if (id < 0 || id >= MAX_FILES) return json(res, errorJson("File id must be a letter A-Z or a number below 5000."), 400);
        sim.doAllocate(id, n);        // an invalid size is reported as "REJECTED" in the result
        json(res, stateJson());
    });

    svr.Post("/api/release", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        int id = WorkloadSimulator::parseFileId(req.get_param_value("id"));
        if (id < 0 || id >= MAX_FILES) return json(res, errorJson("File id must be a letter A-Z or a number below 5000."), 400);
        sim.doRelease(id);
        json(res, stateJson());
    });

    // Which segment would each strategy choose for a request of this size? (pure search, changes nothing)
    svr.Get("/api/preview", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        int n = toInt(req.get_param_value("size"), 0);
        std::ostringstream o;
        o << "{\"size\":" << n;
        const char* keys[3] = {"first", "best", "worst"};
        for (int k = 0; k < 3; k++) {
            FreeSegment* seg = n > 0 ? AllocationManager::chooseSegment(*sim.fl, n, ALL[k]) : nullptr;
            o << ",\"" << keys[k] << "\":" << (seg ? seg->start : -1);
        }
        o << "}";
        json(res, o.str());
    });

    // Apply a random workload to the CURRENT storage (quick way to create fragmentation)
    svr.Post("/api/random", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        int count = toInt(req.get_param_value("count"), 12);
        int seed = toInt(req.get_param_value("seed"), 1);
        int maxSize = toInt(req.get_param_value("max"), sim.total / 6);
        if (count < 1 || count > 300) return json(res, errorJson("count must be 1-300"), 400);
        if (maxSize < 1) maxSize = 1;
        if (sim.nextAutoId + count >= MAX_FILES) return json(res, errorJson("Too many random files. Press Reset first."), 400);
        std::vector<Op> ops = WorkloadSimulator::generateWorkload(count, (unsigned)seed, 1, maxSize, 0.6);
        int base = sim.nextAutoId;
        int maxId = 0;
        for (const Op& o : ops) {
            if (o.isAlloc) sim.doAllocate(base + o.id, o.size);
            else sim.doRelease(base + o.id);
            if (o.id > maxId) maxId = o.id;
        }
        sim.nextAutoId = base + maxId + 1;
        sim.last = LastAction();
        sim.last.type = "random";
        sim.last.message = "Applied " + std::to_string(count) + " random operations";
        json(res, stateJson());
    });

    // Run the same random workload through all three strategies and return the results
    svr.Post("/api/compare", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        int blocks = toInt(req.get_param_value("blocks"), 500);
        int nops = toInt(req.get_param_value("ops"), 200);
        int seed = toInt(req.get_param_value("seed"), 1);
        int maxSize = toInt(req.get_param_value("max"), 20);
        if (blocks < 10 || blocks > 20000) return json(res, errorJson("Blocks must be 10-20000"), 400);
        if (nops < 1 || nops > 20000) return json(res, errorJson("Operations must be 1-20000"), 400);
        if (maxSize < 1 || maxSize > blocks) return json(res, errorJson("Max request size must be between 1 and the number of blocks"), 400);
        std::vector<Op> ops = WorkloadSimulator::generateWorkload(nops, (unsigned)seed, 1, maxSize, 0.6);
        std::ostringstream o;
        o << "{\"blocks\":" << blocks << ",\"ops\":" << nops << ",\"seed\":" << seed << ",\"max\":" << maxSize << ",\"rows\":[";
        for (int k = 0; k < 3; k++) {
            RunResult r = WorkloadSimulator::run(ops, blocks, ALL[k], false, true);
            if (k) o << ",";
            o << rowJson(r);
        }
        o << "]}";
        json(res, o.str());
    });

    // Small / medium / large experiment, 5 seeds each, averaged. Also writes results/results.csv
    svr.Post("/api/experiment", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> g(mtx);
        struct Cfg { const char* name; int blocks, ops, minS, maxS; };
        Cfg cfgs[3] = {{"Small", 50, 20, 1, 6}, {"Medium", 500, 200, 1, 20}, {"Large", 5000, 2000, 1, 100}};
        const int SEEDS = 5;
        std::ofstream csv("results/results.csv");
        bool wroteCsv = (bool)csv;
        if (wroteCsv)
            csv << "size,blocks,ops,seed,strategy,success,failFrag,failNoSpace,badReleases,freeBlocks,freeSegments,largest,finalFrag,avgFrag,timeUs,consistent\n";
        std::ostringstream o;
        o << "{\"csv\":" << (wroteCsv ? "true" : "false") << ",\"seeds\":" << SEEDS << ",\"sizes\":[";
        for (int c = 0; c < 3; c++) {
            const Cfg& cf = cfgs[c];
            double sum[3][8] = {};
            bool allOk[3] = {true, true, true};
            for (int seed = 1; seed <= SEEDS; seed++) {
                std::vector<Op> ops = WorkloadSimulator::generateWorkload(cf.ops, (unsigned)seed, cf.minS, cf.maxS, 0.6);
                for (int k = 0; k < 3; k++) {
                    RunResult r = WorkloadSimulator::run(ops, cf.blocks, ALL[k], false, true);
                    if (wroteCsv)
                        csv << cf.name << "," << cf.blocks << "," << cf.ops << "," << seed << "," << strategyName(ALL[k]) << ","
                            << r.success << "," << r.failFrag << "," << r.failNoSpace << "," << r.badReleases << ","
                            << r.freeBlocks << "," << r.freeSegments << "," << r.largest << ","
                            << r.finalFrag << "," << r.avgFrag << "," << r.timeUs << "," << (r.consistent ? 1 : 0) << "\n";
                    sum[k][0] += r.success; sum[k][1] += r.failFrag; sum[k][2] += r.failNoSpace;
                    sum[k][3] += r.freeSegments; sum[k][4] += r.largest; sum[k][5] += r.avgFrag;
                    sum[k][6] += r.timeUs; sum[k][7] += r.finalFrag;
                    if (!r.consistent) allOk[k] = false;
                }
            }
            if (c) o << ",";
            o << "{\"name\":\"" << cf.name << "\",\"blocks\":" << cf.blocks << ",\"ops\":" << cf.ops << ",\"rows\":[";
            for (int k = 0; k < 3; k++) {
                if (k) o << ",";
                o << "{\"strategy\":\"" << strategyKey(ALL[k]) << "\",\"name\":\"" << strategyName(ALL[k]) << "\""
                  << ",\"success\":" << num(sum[k][0] / SEEDS) << ",\"failFrag\":" << num(sum[k][1] / SEEDS)
                  << ",\"failNoSpace\":" << num(sum[k][2] / SEEDS) << ",\"freeSegments\":" << num(sum[k][3] / SEEDS)
                  << ",\"largest\":" << num(sum[k][4] / SEEDS) << ",\"avgFrag\":" << num(sum[k][5] / SEEDS)
                  << ",\"finalFrag\":" << num(sum[k][7] / SEEDS) << ",\"timeUs\":" << num(sum[k][6] / SEEDS)
                  << ",\"consistent\":" << (allOk[k] ? "true" : "false") << "}";
            }
            o << "]}";
        }
        o << "]}";
        json(res, o.str());
    });

    std::cout << "=====================================================\n"
              << " Virtual Storage Simulator is running.\n"
              << " Open this address in your browser:\n\n"
              << "     http://localhost:" << port << "\n\n"
              << " (Press Ctrl+C in this window to stop the program)\n"
              << "=====================================================\n";
if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "ERROR: could not start on port " << port << ". Is another copy still running?\n"
                  << "Close it, or try another port:  gui_server 8090\n";
        return 1;
    }
    return 0;
}
