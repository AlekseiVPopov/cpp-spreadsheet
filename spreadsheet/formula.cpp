#include "formula.h"

#include "FormulaAST.h"
#include "sheet.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream &operator<<(std::ostream &output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
// Реализуйте следующие методы:
        explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
        }

        Value Evaluate(const SheetInterface &sheet) const override {
            try {
                std::function<double(const Position)> GetValue = [&sheet](const Position pos) -> double {
                    const auto cell = sheet.GetCell(pos);
                    if (!cell) {
                        if (pos.IsValid()) { return 0.0; }
                        else { throw FormulaError{FormulaError::Category::Ref}; }
                    }

                    return std::visit([](auto &&arg) -> double {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, double>) { return arg; }
                        else if constexpr (std::is_same_v<T, std::string>) {
                            if (arg.empty()) { return 0.0; }
                            else if (double d{};(std::istringstream(arg) >> d >> std::ws).eof()) { return d; }
                            else { throw FormulaError{FormulaError::Category::Value}; }
                        } else if constexpr (std::is_same_v<T, FormulaError>) {
                            throw arg;
                        }
                    }, cell->GetValue());
                };

                return ast_.Execute(GetValue);
            } catch (const FormulaError &evaluate_error) {
                return evaluate_error;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const override;

    private:
        FormulaAST ast_;
    };

    std::vector<Position> Formula::GetReferencedCells() const {
        auto cells = ast_.GetCells();
        cells.unique();
        std::vector<Position> res{cells.begin(), cells.end()};
        return res;
    }
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}