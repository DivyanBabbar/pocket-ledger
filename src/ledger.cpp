#include "ledger.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace pocket {
namespace {

std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (quoted && ch == '"' && i + 1 < line.size() && line[i + 1] == '"') {
            field += '"';
            ++i;
        } else if (ch == '"') {
            quoted = !quoted;
        } else if (ch == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
        } else {
            field += ch;
        }
    }
    if (quoted) throw std::invalid_argument("Unclosed CSV quote");
    fields.push_back(field);
    return fields;
}

std::string csvEscape(const std::string& value) {
    std::string escaped;
    bool needsQuotes = false;
    for (char ch : value) {
        if (ch == '"') escaped += "\"\"";
        else escaped += ch;
        needsQuotes = needsQuotes || ch == ',' || ch == '"' || ch == '\n' || ch == '\r';
    }
    return needsQuotes ? '"' + escaped + '"' : escaped;
}

int parsePositiveId(const std::string& value) {
    std::size_t used = 0;
    const int id = std::stoi(value, &used);
    if (used != value.size() || id < 1) throw std::invalid_argument("Invalid ID");
    return id;
}

} // namespace

bool validDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i != 4 && i != 7 && !std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }
    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || month < 1 || month > 12) return false;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= days[month - 1];
}

bool parseMoney(const std::string& text, long long& cents) {
    if (text.empty()) return false;
    std::size_t dot = text.find('.');
    const std::string whole = dot == std::string::npos ? text : text.substr(0, dot);
    const std::string fraction = dot == std::string::npos ? "" : text.substr(dot + 1);
    if (whole.empty() || fraction.size() > 2 || (dot != std::string::npos && fraction.empty())) return false;
    if (!std::all_of(whole.begin(), whole.end(), [](unsigned char c) { return std::isdigit(c); })) return false;
    if (!std::all_of(fraction.begin(), fraction.end(), [](unsigned char c) { return std::isdigit(c); })) return false;
    try {
        const long long rupees = std::stoll(whole);
        const int paise = fraction.empty() ? 0 : std::stoi(fraction + (fraction.size() == 1 ? "0" : ""));
        if (rupees > (std::numeric_limits<long long>::max() - paise) / 100) return false;
        cents = rupees * 100 + paise;
        return cents > 0;
    } catch (const std::exception&) {
        return false;
    }
}

std::string formatMoney(long long cents) {
    const bool negative = cents < 0;
    const unsigned long long magnitude = negative ? static_cast<unsigned long long>(-(cents + 1)) + 1 : static_cast<unsigned long long>(cents);
    std::ostringstream out;
    if (negative) out << '-';
    out << "Rs " << magnitude / 100 << '.' << std::setw(2) << std::setfill('0') << magnitude % 100;
    return out.str();
}

std::string kindName(Kind kind) { return kind == Kind::Income ? "income" : "expense"; }

Ledger::Ledger(std::filesystem::path file) : file_(std::move(file)) {}

std::vector<std::string> Ledger::load() {
    entries_.clear();
    nextId_ = 1;
    std::vector<std::string> warnings;
    std::ifstream input(file_);
    if (!input) return warnings;
    std::string line;
    if (!std::getline(input, line) || line != "id,date,type,category,description,cents") {
        warnings.push_back("The CSV header is missing or unrecognized; the file was not loaded.");
        return warnings;
    }
    int lineNumber = 1;
    while (std::getline(input, line)) {
        ++lineNumber;
        try {
            const auto fields = parseCsvRow(line);
            if (fields.size() != 6) throw std::invalid_argument("Expected six columns");
            Entry entry;
            entry.id = parsePositiveId(fields[0]);
            if (entry.id == std::numeric_limits<int>::max()) throw std::invalid_argument("ID is too large");
            entry.date = fields[1];
            if (!validDate(entry.date)) throw std::invalid_argument("Invalid date");
            if (fields[2] == "income") entry.kind = Kind::Income;
            else if (fields[2] == "expense") entry.kind = Kind::Expense;
            else throw std::invalid_argument("Invalid type");
            entry.category = fields[3];
            entry.description = fields[4];
            std::size_t used = 0;
            entry.cents = std::stoll(fields[5], &used);
            if (used != fields[5].size() || entry.cents < 1) throw std::invalid_argument("Invalid amount");
            if (std::any_of(entries_.begin(), entries_.end(), [&](const Entry& old) { return old.id == entry.id; }))
                throw std::invalid_argument("Duplicate ID");
            entries_.push_back(entry);
            nextId_ = std::max(nextId_, entry.id + 1);
        } catch (const std::exception& error) {
            warnings.push_back("Skipped CSV line " + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    return warnings;
}

void Ledger::save() const {
    if (!file_.parent_path().empty()) std::filesystem::create_directories(file_.parent_path());
    std::ofstream output(file_, std::ios::trunc);
    if (!output) throw std::runtime_error("Could not open the ledger file for writing.");
    output << "id,date,type,category,description,cents\n";
    for (const auto& entry : entries_) {
        output << entry.id << ',' << csvEscape(entry.date) << ',' << kindName(entry.kind) << ','
               << csvEscape(entry.category) << ',' << csvEscape(entry.description) << ',' << entry.cents << '\n';
    }
    output.flush();
    if (!output) throw std::runtime_error("Could not finish writing the ledger file.");
}

int Ledger::add(Entry entry) {
    if (!validDate(entry.date)) throw std::invalid_argument("Date must be a real date in YYYY-MM-DD format.");
    if (entry.category.empty() || entry.category.size() > 60) throw std::invalid_argument("Category must be 1 to 60 characters.");
    if (entry.description.size() > 160) throw std::invalid_argument("Description must be at most 160 characters.");
    if (entry.cents < 1) throw std::invalid_argument("Amount must be greater than zero.");
    if (nextId_ == std::numeric_limits<int>::max()) throw std::invalid_argument("No more entry IDs are available.");
    entry.id = nextId_++;
    entries_.push_back(entry);
    return entry.id;
}

bool Ledger::erase(int id) {
    const auto found = std::find_if(entries_.begin(), entries_.end(), [id](const Entry& entry) { return entry.id == id; });
    if (found == entries_.end()) return false;
    entries_.erase(found);
    return true;
}

Summary Ledger::summary(const std::string& month) const {
    Summary result;
    for (const auto& entry : entries_) {
        if (!month.empty() && entry.date.substr(0, 7) != month) continue;
        ++result.entryCount;
        if (entry.kind == Kind::Income) result.incomeCents += entry.cents;
        else result.expenseCents += entry.cents;
    }
    return result;
}

} // namespace pocket
