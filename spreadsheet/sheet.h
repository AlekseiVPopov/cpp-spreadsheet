#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

struct PositionHash {
    std::size_t operator()(const Position &pos) const {
        return std::hash<int>{}(pos.row) + 37 * std::hash<int>{}(pos.col);
    }
};

using SheetData = std::unordered_map<Position, std::unique_ptr<Cell>, PositionHash>;

class Sheet : public SheetInterface {
public:

    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface *GetCell(Position pos) const override;

    CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;

    void PrintTexts(std::ostream &output) const override;

    // Можете дополнить ваш класс нужными полями и методами

    const Cell *GetCellPtr(Position pos) const;

    Cell *GetCellPtr(Position pos);

private:
    SheetData data_;
};