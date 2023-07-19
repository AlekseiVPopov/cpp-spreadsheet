#include "common.h"

#include <cctype>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;


const static std::string_view DIV_ERR = "#DIV/0!";
const static std::string_view VALUE_ERR = "#VALUE!";
const static std::string_view REF_ERR = "#REF!";

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    return row >= 0 && col >= 0 && row < MAX_ROWS && col < MAX_COLS;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return "";
    }

    std::string result;
    result.reserve(MAX_POSITION_LENGTH);
    int c = col;
    while (c >= 0) {
        result.insert(result.begin(), static_cast<char>('A' + c % LETTERS));
        c = c / LETTERS - 1;
    }

    result += std::to_string(row + 1);

    return result;
}

Position Position::FromString(std::string_view str) {
    std::match_results<std::string_view::const_iterator> match_results;
    std::regex match_parts("(?:(^[A-Z]{1,3})(\\d{1,5})$)");

    if (!std::regex_match(str.begin(), str.end(), match_results, match_parts)) {
        return NONE;
    }

    auto letters = match_results[1].str();
    auto digits = match_results[2].str();

    int rows = 0, cols = 0, pow = 0;

    for (auto rev_it = letters.rbegin(); rev_it != letters.rend(); ++rev_it) {
        cols += static_cast<int>((*rev_it - 64) * std::pow(LETTERS, pow++));
    }

    rows = std::stoi(digits) - 1;
    Position res{rows, cols - 1};

    return res.IsValid() ? res : NONE;
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}

FormulaError::FormulaError(FormulaError::Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const { return category_; }

bool FormulaError::operator==(FormulaError rhs) const { return category_ == rhs.category_; }

std::string_view FormulaError::ToString() const {
    switch(category_) {
        case Category::Ref: return REF_ERR;
        case Category::Value: return VALUE_ERR;
        case Category::Div0: return DIV_ERR;
    }
    return {};
}

