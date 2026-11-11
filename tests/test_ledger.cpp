#include "ledger.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int checks = 0;
void require(bool condition, const std::string& message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        long long cents = 0;
        require(pocket::parseMoney("12.5", cents) && cents == 1250, "money parsing with one decimal");
        require(pocket::parseMoney("0.01", cents) && cents == 1, "money parsing paise");
        require(!pocket::parseMoney("-4", cents), "negative amount rejected");
        require(!pocket::parseMoney("4.999", cents), "fraction over two digits rejected");
        require(pocket::formatMoney(-1) == "-Rs 0.01", "negative balance formatting");
        require(pocket::validDate("2024-02-29"), "leap date accepted");
        require(!pocket::validDate("2023-02-29"), "invalid leap date rejected");
        require(!pocket::validDate("2024-13-01"), "invalid month rejected");

        const auto file = std::filesystem::temp_directory_path() / "pocket-ledger-test.csv";
        std::error_code ignored;
        std::filesystem::remove(file, ignored);
        pocket::Ledger ledger(file);
        pocket::Entry income{0, "2026-01-02", pocket::Kind::Income, "Work", "Pay, January", 10000};
        pocket::Entry expense{0, "2026-01-03", pocket::Kind::Expense, "Food", "Lunch", 1250};
        const int first = ledger.add(income);
        const int second = ledger.add(expense);
        ledger.save();

        pocket::Ledger loaded(file);
        require(loaded.load().empty(), "saved file loads cleanly");
        require(loaded.entries().size() == 2, "entries round trip");
        require(loaded.entries()[0].description == "Pay, January", "CSV comma round trips");
        const auto month = loaded.summary("2026-01");
        require(month.incomeCents == 10000 && month.expenseCents == 1250, "monthly totals");
        require(month.balanceCents() == 8750 && month.entryCount == 2, "monthly balance and count");
        require(loaded.erase(second), "delete existing entry");
        require(!loaded.erase(999), "delete missing entry returns false");
        require(first == 1, "IDs start at one");
        loaded.save();
        pocket::Ledger reloaded(file);
        require(reloaded.load().empty() && reloaded.entries().size() == 1, "delete persists");

        std::filesystem::remove(file, ignored);
        std::cout << "Passed " << checks << " ledger checks.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
