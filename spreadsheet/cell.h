#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>
#include <unordered_map>
#include <queue>


class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet &sheet, Position pos);

    ~Cell() override;

    void Set(std::string text);

    void Clear();

    Value GetValue() const override;

    std::string GetText() const override;

    bool IsReferenced() const;

private:

    std::vector<Position> GetReferencedCells() const override;

    Position GetPosition() const;

    void AddAffected(Cell *cell);

    void RemoveAffected(Cell *cell);

    const std::deque<Cell *> &GetAffectOn();

    void ClearCache(bool only_local = true) const;

    class Impl;

    class EmptyImpl;

    class TextImpl;

    class FormulaImpl;

    Sheet &sheet_;
    Position position_;
    std::unique_ptr<Impl> impl_;
    std::deque<Cell *> affect_on_;

};