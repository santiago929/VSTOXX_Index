#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <optional>
#include <filesystem>

const double SECONDS_YEAR = 365.0 * 24 * 3600.0;
const double SECONDS_30_DAYS = 30.0 * 24 * 3600.0;

struct Row {
    std::tm date{};
    std::optional<double> V6I1;
    std::optional<double> V6I2;
    std::optional<double> V6I3;
    double V2TX{};

    double vstoxx{};
    double difference{};
};

std::tm parse_date(const std::string& s) {
    std::tm tm{};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    return tm;
}

time_t to_time_t(std::tm t) {
    t.tm_isdst = -1;
    return std::mktime(&t);
}

int days_between(std::tm a, std::tm b) {
    return static_cast<int>((to_time_t(b) - to_time_t(a)) / (24 * 3600));
}

std::tm add_days(std::tm date, int days) {
    time_t t = to_time_t(date);
    t += days * 24 * 3600;

    std::tm result{};
    localtime_s(&result, &t);   

    return result;
}

int weekday(std::tm date) {
    time_t t = to_time_t(date);

    std::tm result{};
    localtime_s(&result, &t);   

    return result.tm_wday == 0 ? 6 : result.tm_wday - 1;
}

std::tm third_friday(std::tm date) {
    std::tm first = date;
    first.tm_mday = 1;
    std::mktime(&first);

    int wd = weekday(first);
    int delta = 4 - wd;
    if (delta < 0) delta += 7;

    return add_days(first, delta + 14);
}

std::tm first_settlement_day(std::tm date) {
    std::tm settlement = third_friday(date);
    int delta = days_between(date, settlement);

    if (delta > 1)
        return settlement;
    else
        return third_friday(add_days(settlement, 20));
}

std::tm second_settlement_day(std::tm date) {
    std::tm first = first_settlement_day(date);
    return third_friday(add_days(first, 20));
}

void calculate_vstoxx(const std::string& filename) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file.\n";
        return;
    }

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {

        std::stringstream ss(line);
        std::string token;
        Row row;

        std::getline(ss, token, ',');
        row.date = parse_date(token);

        auto parse_optional = [](const std::string& s) -> std::optional<double> {
            if (s.empty()) return std::nullopt;
            return std::stod(s);
            };

        std::getline(ss, token, ',');
        row.V6I1 = parse_optional(token);

        std::getline(ss, token, ',');
        row.V6I2 = parse_optional(token);

        std::getline(ss, token, ',');
        row.V6I3 = parse_optional(token);

        std::getline(ss, token, ',');
        row.V2TX = std::stod(token);

        std::tm s1 = first_settlement_day(row.date);
        std::tm s2 = second_settlement_day(row.date);

        double life1 = days_between(row.date, s1) * 24 * 3600.0;
        double life2 = days_between(row.date, s2) * 24 * 3600.0;

        if (!row.V6I2.has_value() || !row.V6I3.has_value())
            continue;

        double sub1, sub2;

        if (row.V6I1.has_value()) {
            sub1 = row.V6I1.value();
            sub2 = row.V6I2.value();
        }
        else {
            sub1 = row.V6I2.value();
            sub2 = row.V6I3.value();
        }

        double part1 =
            life1 / SECONDS_YEAR *
            sub1 * sub1 *
            ((life2 - SECONDS_30_DAYS) /
                (life2 - life1));

        double part2 =
            life2 / SECONDS_YEAR *
            sub2 * sub2 *
            ((SECONDS_30_DAYS - life1) /
                (life2 - life1));

        row.vstoxx = std::sqrt((part1 + part2) *
            SECONDS_YEAR / SECONDS_30_DAYS);

        row.difference = row.V2TX - row.vstoxx;

        std::cout << "Calculated: " << row.vstoxx
            << "  Market: " << row.V2TX
            << "  Diff: " << row.difference << "\n";
    }
}

int main() {

    std::filesystem::path path =
        std::filesystem::current_path() / "vstoxx_data.csv";

    //std::cout << "Looking for file at: " << path << "\n";

    calculate_vstoxx(path.string());

    return 0;
}

