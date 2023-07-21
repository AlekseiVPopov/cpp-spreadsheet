#include "cell.h"
#include "sheet.h"

#include <iostream>
#include <string>
#include <optional>
#include <queue>
#include <unordered_set>


// Реализуйте следующие методы

class Cell::Impl {
public:
    Impl(Sheet *sheet, Cell *cell) : sheet_(sheet), cell_(cell) {}

    virtual Value GetValue() const = 0;

    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const;

    virtual std::vector<Cell *> GetReferencedCellsPtr() const;

    virtual bool HasCircularDependency() const;

    virtual void ClearCache() const;

    virtual void AddDependencies() const;

    virtual void RemoveDependencies() const;

    virtual ~Impl() = default;

protected:
    Sheet *sheet_;
    Cell *cell_;
};

class Cell::EmptyImpl : public Impl {
public:
    EmptyImpl(Sheet *sheet, Cell *cell) : Impl(sheet, cell) {}

    Value GetValue() const override;

    std::string GetText() const override;

};

class Cell::TextImpl : public Impl {
public:

    explicit TextImpl(std::string text, Sheet *sheet, Cell *cell) : Impl(sheet, cell), text_(std::move(text)) {}

    Value GetValue() const override;

    std::string GetText() const override;

private:
    std::string text_;
};


class Cell::FormulaImpl : public Impl {
public:

    FormulaImpl(std::string text, Sheet *sheet, Cell *cell);

    Value GetValue() const override;

    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    std::vector<Cell *> GetReferencedCellsPtr() const override;

    bool HasCircularDependency() const override;

    void ClearCache() const override;

    void AddDependencies() const override;

    void RemoveDependencies() const override;

private:
    std::unique_ptr<FormulaInterface> formula_ptr_;
    std::deque<Cell *> depend_on_;
    mutable std::optional<Cell::Value> cache_;
};

Cell::Cell(Sheet &sheet, Position pos) : sheet_(sheet), position_(pos),
                                         impl_(std::make_unique<EmptyImpl>(&sheet_, this)) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text == impl_->GetText()) return;

    try {
        std::unique_ptr<Cell::Impl> temp;

        if (text.empty()) {
            temp = std::make_unique<EmptyImpl>(&sheet_, this);
        } else if (text.size() > 1 && text.at(0) == FORMULA_SIGN) {
            temp = std::make_unique<FormulaImpl>(std::move(text), &sheet_, this);
        } else {
            temp = std::make_unique<TextImpl>(std::move(text), &sheet_, this);
        }

        if (IsReferenced()) {
            ClearCache();
        }
        impl_->RemoveDependencies();
        impl_ = std::move(temp);
        impl_->AddDependencies();
    } catch (...) {
        throw;
    }
}

void Cell::Clear() {
    Set("");
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
            const auto cell = sheet_->GetCellPtr(pos);
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
}

void Cell::FormulaImpl::ClearCache() const {
    cache_.reset();
}

void Cell::FormulaImpl::RemoveDependencies() const {
    for (const auto &cell: depend_on_) {
        cell->RemoveAffected(cell_);
    }
}

std::vector<Cell *> Cell::FormulaImpl::GetReferencedCellsPtr() const {
    return std::vector<Cell *>{depend_on_.begin(), depend_on_.end()};
}

void Cell::FormulaImpl::AddDependencies() const {
    for (const auto dep_cell: depend_on_) {
        dep_cell->AddAffected(cell_);
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

std::vector<Cell *> Cell::Impl::GetReferencedCellsPtr() const {
    return {};
}

void Cell::Impl::AddDependencies() const {}

void Cell::AddAffected(Cell *cell) {
    if (std::find(affect_on_.begin(), affect_on_.end(), cell) == affect_on_.end()) {
        affect_on_.insert(cell);
    }
}

void Cell::RemoveAffected(Cell *cell) {
    affect_on_.erase(cell);
}

void Cell::ClearCache() const {
    impl_->ClearCache();

    for (const auto &cell: affect_on_) {
        cell->ClearCache();
    }
}