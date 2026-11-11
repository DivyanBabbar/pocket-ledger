#include "ledger.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string ask(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

void printEntries(const pocket::Ledger& ledger) {
    if (ledger.entries().empty()) {
        std::cout << "No entries yet.\n";
        return;
    }
    std::cout << "\nID   DATE       TYPE     CATEGORY           AMOUNT       DESCRIPTION\n";
    std::cout << "--------------------------------------------------------------------------\n";
    for (const auto& entry : ledger.entries()) {
        std::cout << entry.id << "   " << entry.date << "  " << pocket::kindName(entry.kind) << "  "
                  << entry.category << "  " << pocket::formatMoney(entry.cents) << "  " << entry.description << '\n';
    }
}

void addEntry(pocket::Ledger& ledger, pocket::Kind kind) {
    pocket::Entry entry;
    entry.kind = kind;
    entry.date = ask("Date (YYYY-MM-DD): ");
    entry.category = ask("Category: ");
    entry.description = ask("Description (optional): ");
    const auto amount = ask("Amount in rupees (e.g. 125.50): ");
    if (!pocket::parseMoney(amount, entry.cents)) {
        std::cout << "Enter an amount greater than zero with at most two decimal places.\n";
        return;
    }
    try {
        const int id = ledger.add(entry);
        ledger.save();
        std::cout << "Saved entry #" << id << ".\n";
    } catch (const std::exception& error) {
        std::cout << error.what() << '\n';
    }
}

void printSummary(const pocket::Ledger& ledger) {
    std::string month = ask("Month (YYYY-MM, or blank for all time): ");
    const bool monthFormat = month.size() == 7 && month[4] == '-' &&
        std::all_of(month.begin(), month.begin() + 4, [](unsigned char c) { return std::isdigit(c); }) &&
        std::all_of(month.begin() + 5, month.end(), [](unsigned char c) { return std::isdigit(c); });
    const int monthNumber = monthFormat ? std::stoi(month.substr(5, 2)) : 0;
    if (!month.empty() && (!monthFormat || monthNumber < 1 || monthNumber > 12)) {
        std::cout << "Use YYYY-MM or leave blank.\n";
        return;
    }
    const auto summary = ledger.summary(month);
    std::cout << "Entries:  " << summary.entryCount << '\n'
              << "Income:   " << pocket::formatMoney(summary.incomeCents) << '\n'
              << "Expenses: " << pocket::formatMoney(summary.expenseCents) << '\n'
              << "Balance:  " << pocket::formatMoney(summary.balanceCents()) << '\n';
}

} // namespace

int main() {
    const char* configuredPath = std::getenv("POCKET_LEDGER_FILE");
    pocket::Ledger ledger(configuredPath && *configuredPath ? configuredPath : "ledger.csv");
    for (const auto& warning : ledger.load()) std::cerr << "Warning: " << warning << '\n';
    std::cout << "Pocket Ledger - local CSV income and expense tracker\n";

    while (true) {
        std::cout << "\n1) Add income\n2) Add expense\n3) List entries\n4) Monthly summary\n5) Delete entry\n0) Save and quit\n";
        const auto choice = ask("Choose: ");
        if (choice == "1") addEntry(ledger, pocket::Kind::Income);
        else if (choice == "2") addEntry(ledger, pocket::Kind::Expense);
        else if (choice == "3") printEntries(ledger);
        else if (choice == "4") printSummary(ledger);
        else if (choice == "5") {
            const auto rawId = ask("Entry ID to delete: ");
            try {
                std::size_t used = 0;
                const int id = std::stoi(rawId, &used);
                if (used != rawId.size() || id < 1) throw std::invalid_argument("invalid ID");
                if (ledger.erase(id)) { ledger.save(); std::cout << "Entry deleted.\n"; }
                else std::cout << "No entry with that ID.\n";
            } catch (const std::exception&) { std::cout << "Enter a valid numeric ID.\n"; }
        } else if (choice == "0") {
            try { ledger.save(); std::cout << "Saved. Goodbye.\n"; }
            catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
            return 0;
        } else std::cout << "Choose one of the listed options.\n";
    }
}
