#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <cctype>
using namespace std;

struct Cake {
    string ten;
};

struct CakeTxn {
    int transactionNo;
    string items;
    string datetime;
    string daypart;
    string daytype;
};

static inline string trim(string s) {
    auto notspace = [](int ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
    return s;
}

static vector<string> splitCSVLine(const string& line) {
    vector<string> out;
    string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        }
        else if (c == ',' && !inQuotes) {
            out.push_back(trim(cur));
            cur.clear();
        }
        else {
            cur.push_back(c);
        }
    }
    out.push_back(trim(cur));
    return out;
}

static bool safe_to_int(const string& s, int& val) {
    try {
        size_t idx = 0;
        string t = trim(s);
        long long v = stoll(t, &idx);
        if (idx != t.size()) return false;
        if (v < numeric_limits<int>::min() || v > numeric_limits<int>::max()) return false;
        val = static_cast<int>(v);
        return true;
    }
    catch (...) { return false; }
}

struct LoadOptions {
    bool drop_header = true;
    bool dedupe_rows = true;
};

bool load_transactions_from_file(const string& path, vector<CakeTxn>& rows, const LoadOptions& opt = {}) {
    ifstream f(path);
    if (!f.is_open()) {
        cerr << "Khong mo duoc file: " << path << "\n";
        cerr << "Hay kiem tra quyen truy cap file va duong dan.\n";
        return false;
    }
    string line;
    bool header_checked = false;
    size_t ok = 0, skipped_blank = 0, skipped_bad = 0, skipped_dup = 0;
    unordered_set<string> seen;
    while (getline(f, line)) {
        string raw = trim(line);
        if (raw.empty()) { ++skipped_blank; continue; }
        if (!header_checked && opt.drop_header) {
            header_checked = true;
            string low = raw;
            for (auto& ch : low) ch = (char)tolower((unsigned char)ch);
            if (low.find("transactionno") != string::npos &&
                low.find("items") != string::npos &&
                low.find("datetime") != string::npos) {
                continue;
            }
        }
        auto cols = splitCSVLine(raw);
        if (cols.size() < 5) {
            cerr << "Dong khong du 5 cot: " << raw << "\n";
            ++skipped_bad;
            continue;
        }
        int txn = 0;
        if (!safe_to_int(cols[0], txn)) {
            cerr << "transactionNo khong hop le: " << cols[0] << "\n";
            ++skipped_bad;
            continue;
        }
        string items = trim(cols[1]);
        string dt = trim(cols[2]);
        string daypart = trim(cols[3]);
        string daytype = trim(cols[4]);
        if (items.empty()) {
            cerr << "Items trong: " << raw << "\n";
            ++skipped_bad;
            continue;
        }
        if (opt.dedupe_rows) {
            string key = to_string(txn) + "|" + items + "|" + dt + "|" + daypart + "|" + daytype;
            if (seen.count(key)) {
                ++skipped_dup;
                continue;
            }
            seen.insert(key);
        }
        rows.push_back(CakeTxn{txn, items, dt, daypart, daytype});
        ++ok;
    }
    f.close();
    cout << "Nap OK: " << ok
         << " | Bo dong trong: " << skipped_blank
         << " | Bo dong hong/le thieu cot: " << skipped_bad
         << " | Bo dong trung: " << skipped_dup << "\n";
    return true;
}

vector<Cake> to_cakes_unique_by_name(const vector<CakeTxn>& rows) {
    vector<Cake> cakes;
    unordered_set<string> seen;
    for (const auto& r : rows) {
        string name = r.items;
        if (seen.insert(name).second) {
            cakes.push_back(Cake{name});
        }
    }
    return cakes;
}

vector<Cake> to_cakes_all(const vector<CakeTxn>& rows) {
    vector<Cake> cakes;
    cakes.reserve(rows.size());
    for (const auto& r : rows) {
        cakes.push_back(Cake{r.items});
    }
    return cakes;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string path = (argc >= 2) ? string(argv[1]) : "D:\\DoAn\\banhkemcsv\\Bakery.csv";
    vector<CakeTxn> txns;
    LoadOptions opt;
    if (!load_transactions_from_file(path, txns, opt)) {
        cerr << "That bai khi doc file CSV.\n";
        return 1;
    }
    cout << "\n=== 5 giao dich dau ===\n";
    for (size_t i = 0; i < txns.size() && i < 100; ++i) {
        const auto& r = txns[i];
        cout << r.transactionNo << " | " << r.items << " | "
             << r.datetime << " | " << r.daypart << " | " << r.daytype << "\n";
    }
    auto cakes_unique = to_cakes_unique_by_name(txns);
    cout << "\n=== Danh sach Cake duy nhat theo ten (Items) ===\n";
    for (const auto& c : cakes_unique) {
        cout << "- " << c.ten << "\n";
    }
    auto cakes_all = to_cakes_all(txns);
    cout << "\nTong so giao dich hop le: " << txns.size() << "\n";
    cout << "So loai Cake duy nhat: " << cakes_unique.size() << "\n";
    cout << "Tong so Cake (bao gom lap): " << cakes_all.size() << "\n";
    return 0;
}