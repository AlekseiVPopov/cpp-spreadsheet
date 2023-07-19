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
    explicit Cell(Sheet &sheet) : sheet_(sheet), impl_(std::make_unique<EmptyImpl>(&sheet_, this)) {}

    ~Cell() override = default;

    void Set(Position pos, std::string text);

    void Clear();

    Value GetValue() const override;

    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

    Position GetPosition() const;

    void AddAffected(const Cell *cell) const;

    void RemoveAffected(const Cell *cell) const;

    void ClearCache(bool only_local = true) const;

private:
    class Impl {
    public:
        Impl(Sheet *sheet, Cell *cell) : sheet_(sheet), cell_(cell) {}

        virtual Value GetValue() const = 0;

        virtual std::string GetText() const = 0;

        virtual std::vector<Position> GetReferencedCells() const;

        virtual bool HasCircularDependency() const;

        virtual void ClearCache() const;

        virtual void RemoveDependencies () const;

        virtual ~Impl() = default;

    protected:
        Sheet *sheet_;
        Cell *cell_;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl(Sheet *sheet, Cell *cell) : Impl(sheet, cell) {}

        Value GetValue() const override;

        std::string GetText() const override;

    };

    class TextImpl : public Impl {
    public:

        explicit TextImpl(std::string text, Sheet *sheet, Cell *cell) : Impl(sheet, cell), text_(std::move(text)) {}

        Value GetValue() const override;

        std::string GetText() const override;

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:

        FormulaImpl(std::string text, Sheet *sheet, Cell *cell);

        Value GetValue() const override;

        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        bool HasCircularDependency() const override;

        void ClearCache() const override;

        void RemoveDependencies() const override;


    private:
        std::unique_ptr<FormulaInterface> formula_ptr_;
        mutable std::deque<const Cell *> depend_on_;
        mutable std::optional<Cell::Value> cache_;
    };

    Sheet &sheet_;
    Position position_;
    std::unique_ptr<Impl> impl_;
    mutable std::deque<const Cell *> affect_on_;


    // Добавьте поля и методы для связи с таблицей, проверки циклических 
    // зависимостей, графа зависимостей и т. д.

};