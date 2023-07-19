#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <queue>
#include <unordered_set>


// Реализуйте следующие методы


void Cell::Set(Position pos, std::string text) {
    position_ = pos;
    if (text == impl_->GetText()) return;

    if (IsReferenced()) {
        ClearCache(false);
    }
    impl_->RemoveDependencies();

    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>(&sheet_, this);
        return;

    } else if (text.size() > 1 && text.at(0) == FORMULA_SIGN) {
        impl_ = std::make_unique<FormulaImpl>(std::move(text), &sheet_, this);
        return;
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text), &sheet_, this);
        return;
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>(&sheet_, this);
}

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }

std::string Cell::GetText() const { return impl_->GetText(); }

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !affect_on_.empty();
}

Position Cell::GetPosition() const {
    return position_;
}

Cell::Value Cell::EmptyImpl::GetValue() const { return 0.0; }

std::string Cell::EmptyImpl::GetText() const { return std::string{}; }


Cell::Value Cell::TextImpl::GetValue() const {
    return text_.at(0) == ESCAPE_SIGN ? text_.substr(1) : text_;
}

std::string Cell::TextImpl::GetText() const { return text_; }

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_.has_value()) {
        const auto res = formula_ptr_->Evaluate(*sheet_);

        if (std::holds_alternative<double>(res)) {
            cache_ = std::make_optional<Cell::Value>(std::get<double>(res));
        } else {
            cache_ = std::make_optional<Cell::Value>(std::get<FormulaError>(res));
        }
    }
    return cache_.value();

//    const auto res = formula_ptr_->Evaluate(*sheet_);
//    if (std::holds_alternative<double>(res)) {
//        return std::get<double>(res);
//    } else {
//        return std::get<FormulaError>(res);
//    }
}

std::string Cell::FormulaImpl::GetText() const { return FORMULA_SIGN + formula_ptr_->GetExpression(); }

bool Cell::FormulaImpl::HasCircularDependency() const {
    std::unordered_set<const Cell *> visited;
    std::queue<const Cell *> queue;

    for (const auto &d: depend_on_) {
        queue.push(d);
    }

    while (!queue.empty()) {
        const auto current_cell = queue.front();
        if (current_cell == cell_) { return true; }
        visited.emplace(current_cell);

        const auto deps_pos = current_cell->GetReferencedCells();
        for (const auto &pos: deps_pos) {
            const auto cell = dynamic_cast<const Cell *>(sheet_->GetCell(pos));
            if (!visited.count(cell)) { queue.push(cell); }
        }
        queue.pop();
    }
    return false;
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    std::vector<Position> res;
    res.reserve(depend_on_.size());

    for (const auto &cell: depend_on_) {
        res.emplace_back(cell->GetPosition());
    }
    return res;
}

Cell::FormulaImpl::FormulaImpl(std::string text, Sheet *sheet, Cell *cell) :
        Impl(sheet, cell), formula_ptr_(
        ParseFormula(text.substr(1))) {
    const auto ref_cells_pos = formula_ptr_->GetReferencedCells();
    for (const auto &pos: ref_cells_pos) {
        auto ref_cell_ptr = sheet_->GetCell(pos);
        if (!ref_cell_ptr) {
            sheet_->SetCell(pos, "");
        }
        depend_on_.push_back(sheet_->GetCellPtr(pos));
    }
    if (this->HasCircularDependency()) {
        throw CircularDependencyException("Formula has circular dependency");
    }

    for (const auto dep_cell: depend_on_) {
        dep_cell->AddAffected(cell_);
    }
}

void Cell::FormulaImpl::ClearCache() const {
    cache_.reset();
}

void Cell::FormulaImpl::RemoveDependencies() const {
    for (const auto &cell: depend_on_) {
        cell->RemoveAffected(cell_);
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

bool Cell::Impl::HasCircularDependency() const {
    return false;
}

void Cell::Impl::ClearCache() const {}

void Cell::Impl::RemoveDependencies() const {}

void Cell::AddAffected(const Cell *cell) const {
    if (std::find(affect_on_.begin(), affect_on_.end(), cell) == affect_on_.end()) { affect_on_.push_back(cell); }
}

void Cell::RemoveAffected(const Cell *cell) const {
    (void) std::remove(affect_on_.begin(), affect_on_.end(), cell);
}

void Cell::ClearCache(bool only_local) const {
    impl_->ClearCache();
    if (!only_local) {
        for (const auto &cell: affect_on_) {
            cell->ClearCache(true);
        }
    }
}
