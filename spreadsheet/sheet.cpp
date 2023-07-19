#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) { throw InvalidPositionException("Invalid position"); }

    auto pos_it = data_.find(pos);
    if (pos_it == data_.end()) {
        pos_it = data_.emplace(pos, std::make_unique<Cell>(*this)).first;
    }
    pos_it->second->Set(pos, std::move(text));

}

const CellInterface *Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) { throw InvalidPositionException("Invalid position"); }

    if (const auto pos_it = data_.find(pos);pos_it == data_.end()) { return nullptr; }
    else { return pos_it->second.get(); }
}

CellInterface *Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) { throw InvalidPositionException("Invalid position"); }

    if (const auto pos_it = data_.find(pos);pos_it == data_.end()) { return nullptr; }
    else { return pos_it->second.get(); }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) { throw InvalidPositionException("Invalid position"); }

    if (const auto pos_it = data_.find(pos); pos_it != data_.end()) {
        data_.erase(pos_it);
    }

}

Size Sheet::GetPrintableSize() const {
    int rows = 0, cols = 0;
    for (const auto &[pos, ptr]: data_) {
        if (ptr) {
            if (pos.row >= rows) { rows = pos.row + 1; }
            if (pos.col >= cols) { cols = pos.col + 1; }
        }
    }
    return {rows, cols};
}

void Sheet::PrintValues(std::ostream &output) const {
    const Size printable_size = GetPrintableSize();
    if (printable_size == Size{0, 0}) { return; }
    bool first_line = true;

    for (int row = 0; row < printable_size.rows; ++row) {
        if (!first_line) { output << "\n"; }
        else { first_line = false; }

        bool first_cell = true;
        for (int col = 0; col < printable_size.cols; ++col) {
            if (!first_cell) { output << "\t"; }
            else { first_cell = false; }

            if (const auto pos_it = data_.find({row, col}); pos_it != data_.end()) {
                const auto val = pos_it->second->GetValue();
                if (std::holds_alternative<double>(val)) {
                    output << std::get<double>(val);
                } else if (std::holds_alternative<FormulaError>(val)) {
                    output << std::get<FormulaError>(val);
                } else if (std::holds_alternative<std::string>(val)) {
                    output << std::get<std::string>(val);
                }
            }
        }
    }
    output << "\n";
}

void Sheet::PrintTexts(std::ostream &output) const {
    const Size printable_size = GetPrintableSize();
    if (printable_size == Size{0, 0}) { return; }

    bool first_line = true;

    for (int row = 0; row < printable_size.rows; ++row) {
        if (!first_line) { output << "\n"; }
        else { first_line = false; }

        bool first_cell = true;
        for (int col = 0; col < printable_size.cols; ++col) {
            if (!first_cell) { output << "\t"; }
            else { first_cell = false; }

            if (const auto pos_it = data_.find({row, col}); pos_it != data_.end()) {
                output << pos_it->second->GetText();
            }
        }
    }
    output << "\n";

}

const Cell *Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) { throw InvalidPositionException("Invalid position"); }

    if (const auto pos_it = data_.find(pos);pos_it == data_.end()) { return nullptr; }
    else { return pos_it->second.get(); }
}


Cell *Sheet::GetCellPtr(Position pos) {
    if (!pos.IsValid()) { throw InvalidPositionException("Invalid position"); }

    if (const auto pos_it = data_.find(pos);pos_it == data_.end()) { return nullptr; }
    else { return pos_it->second.get(); }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}