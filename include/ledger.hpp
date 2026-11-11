#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace pocket {

enum class Kind { Income, Expense };

struct Entry {
    int id{};
    std::string date;
    Kind kind{Kind::Expense};
    std::string category;
    std::string description;
    long long cents{};
};

struct Summary {
    long long incomeCents{};
    long long expenseCents{};
    int entryCount{};
    long long balanceCents() const { return incomeCents - expenseCents; }
};

bool validDate(const std::string& date);
bool parseMoney(const std::string& text, long long& cents);
std::string formatMoney(long long cents);
std::string kindName(Kind kind);

class Ledger {
public:
    explicit Ledger(std::filesystem::path file);

    std::vector<std::string> load();
    void save() const;
    int add(Entry entry);
    bool erase(int id);
    Summary summary(const std::string& month = "") const;
    const std::vector<Entry>& entries() const { return entries_; }

private:
    std::filesystem::path file_;
    std::vector<Entry> entries_;
    int nextId_{1};
};

} // namespace pocket
