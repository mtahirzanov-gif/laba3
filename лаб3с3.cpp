#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cctype>

using namespace std;

// Проверка, является ли строка целым числом
bool isInteger(const string& str) {
    if (str.empty()) return false;
    
    for (char c : str) {
        if (!isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

// Проверка, является ли строка числом с плавающей точкой
bool isFloat(const string& str) {
    if (str.empty()) return false;
    
    bool hasDecimalPoint = false;
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        
        // Разрешаем цифры
        if (isdigit(static_cast<unsigned char>(c))) continue;
        
        // Разрешаем одну десятичную точку
        if (c == '.' && !hasDecimalPoint) {
            hasDecimalPoint = true;
            continue;
        }
        
        // Любой другой символ - ошибка
        return false;
    }
    
    // Строка не может быть просто точкой
    return str != "." && !str.empty();
}

class PayrollException : public runtime_error {
public:
    explicit PayrollException(const string& msg)
        : runtime_error(msg) {}
};

class InvalidRateException : public PayrollException {
public:
    explicit InvalidRateException(const string& msg)
        : PayrollException("Invalid rate: " + msg) {}
};

class DuplicateWorkTypeException : public PayrollException {
public:
    explicit DuplicateWorkTypeException(const string& msg)
        : PayrollException("Duplicate work type: " + msg) {}
};

class EmptyWorkListException : public PayrollException {
public:
    explicit EmptyWorkListException(const string& msg)
        : PayrollException("Work list is empty: " + msg) {}
};


//БАЗОВЫЙ КЛАСС
class IBonusStrategy {
public:
    virtual ~IBonusStrategy() = default;
    virtual double computePay(double basePay) const = 0;
};

//производные классы
class NoBonusStrategy : public IBonusStrategy {
public:
    double computePay(double basePay) const override {
        return basePay;
//другая реализация первого полиморфизма
    }
};

class PercentageBonusStrategy : public IBonusStrategy {
private:
    double bonusPercent;

public:
    explicit PercentageBonusStrategy(double percent)
        : bonusPercent(percent)
    {
        if (bonusPercent < 0) {
            throw InvalidRateException("bonus percent must be >= 0");
        }
        if (bonusPercent > 100.0) {
            throw InvalidRateException("bonus percent cannot exceed 100%");
        }
    }
//полиморфизм
    double computePay(double basePay) const override {
        return basePay * (1.0 + bonusPercent / 100.0);
//одна реализация
    }
};


// БАЗОВЫЙ КЛАСС
class IWorkType {
public:
    virtual ~IWorkType() = default;

    virtual string getName() const = 0;
    virtual double getBasePay() const = 0;
    virtual double getFinalPay() const = 0;
};

// ПРОИЗВОДНЫЙ КЛАСС
class WorkTypeBase : public IWorkType {
private:
    string name;
    double basePay;
    shared_ptr<IBonusStrategy> bonusStrategy;

public:
    WorkTypeBase(const string& name,
        double basePay,
        shared_ptr<IBonusStrategy> strategy)
        : name(name), basePay(basePay), bonusStrategy(strategy)
    {
        if (name.empty()) {
            throw InvalidRateException("work type name must not be empty");
        }
        if (basePay <= 0) {
            throw InvalidRateException("base pay must be > 0");
        }
        if (basePay > 1000000.0) {
            throw InvalidRateException("base pay cannot exceed 1,000,000");
        }
        if (!bonusStrategy) {
            throw InvalidRateException("bonus strategy must not be null");
        }
    }

    string getName() const override {
        return name;
    }

    double getBasePay() const override {
        return basePay;
    }

    double getFinalPay() const override {
        return bonusStrategy->computePay(basePay);
    }
};


class PayrollDepartment {
private:
    vector<shared_ptr<IWorkType>> workTypes;

    bool existsWorkType(const string& name) const {
        for (const auto& w : workTypes) {
            if (w->getName() == name) {
                return true;
            }
        }
        return false;
    }

public:
    PayrollDepartment() = default;

    void addWorkType(const string& name,
        double basePay,
        double bonusPercent = 0.0)
    {
        if (name.size() > 50) {
            cerr << "Предупреждение: название типа работ очень длинное\n";
        }

        if (existsWorkType(name)) {
            throw DuplicateWorkTypeException(
                "work type '" + name + "' already exists");
        }

        shared_ptr<IBonusStrategy> strategy;

        if (bonusPercent == 0.0) {
            strategy = make_shared<NoBonusStrategy>();
        }
        else {
            if (bonusPercent > 100.0) {
                throw InvalidRateException("bonus percent cannot exceed 100%");
            }
            strategy = make_shared<PercentageBonusStrategy>(bonusPercent);
        }

        auto wt = make_shared<WorkTypeBase>(name, basePay, strategy);
        workTypes.push_back(wt);
    }

    double calculateAveragePay() const {
        if (workTypes.empty()) {
            throw EmptyWorkListException("cannot calculate average");
        }        double sum = 0.0;
        for (const auto& w : workTypes) {
            sum += w->getFinalPay();
        }
        return sum / static_cast<double>(workTypes.size());
    }

    void printAll() const {
        if (workTypes.empty()) {
            cout << "Список типов работ пуст.\n";
            return;
        }

        cout << "Текущие типы работ:\n";
        for (const auto& w : workTypes) {
            cout << "  - " << w->getName()
                << " | базовая оплата: " << w->getBasePay()
                << " | с надбавкой: " << w->getFinalPay()
                << '\n';
        }
    }
};

// Функции ввода с проверками
string inputNonEmptyString(const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);

        // Удаляем лишние пробелы
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == string::npos || end == string::npos) {
            cout << "Ошибка: строка не может быть пустой. Попробуйте снова.\n";
            continue;
        }
        s = s.substr(start, end - start + 1);

        if (!s.empty()) return s;
        cout << "Ошибка: строка не может быть пустой. Попробуйте снова.\n";
    }
}

double inputPositiveDouble(const string& prompt) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        
        // Удаляем пробелы в начале и конце
        input.erase(0, input.find_first_not_of(" "));
        input.erase(input.find_last_not_of(" ") + 1);
        
        // Проверяем, что строка является числом с плавающей точкой
        if (!isFloat(input)) {
            cout << "Ошибка! Введите положительное число до 1000000 (разделитель - точка): ";
            continue;
        }
        
        try {
            double num = stod(input);
            
            if (num <= 0) {
                cout << "Ошибка! Введите положительное число больше 0: ";
            } else if (num > 1000000.0) {
                cout << "Ошибка! Введите число не больше 1000000: ";
            } else {
                return num;
            }
        } catch (const exception&) {
            cout << "Ошибка! Введите положительное число до 1000000 (разделитель - точка): ";
        }
    }
}

double inputNonNegativeDouble(const string& prompt) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        
        // Удаляем пробелы в начале и конце
        input.erase(0, input.find_first_not_of(" "));
        input.erase(input.find_last_not_of(" ") + 1);
        
        // Проверяем, что строка является числом с плавающей точкой
        if (!isFloat(input)) {
            cout << "Ошибка! Введите неотрицательное число до 100 (разделитель - точка): ";
            continue;
        }
        
        try {
            double num = stod(input);
            
            if (num < 0) {
                cout << "Ошибка! Введите неотрицательное число: ";
            } else if (num > 100.0) {
                cout << "Ошибка! Введите число не больше 100: ";
            } else {
                return num;
            }
        } catch (const exception&) {
            cout << "Ошибка! Введите неотрицательное число до 100 (разделитель - точка): ";
        }
    }
}

int inputMenuChoice(const string& prompt, int low, int high) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        
        // Проверка на пустую строку
        if (line.empty()) {
            cout << "Ошибка: введите число от " << low << " до " << high << ".\n";
            continue;
        }
        
        // Удаляем пробелы
        line.erase(0, line.find_first_not_of(" "));
        line.erase(line.find_last_not_of(" ") + 1);
        
        // Проверка, что все символы - цифры
        bool allDigits = !line.empty() &&
            all_of(line.begin(), line.end(), [](unsigned char c) {
                return isdigit(c);
            });

        if (!allDigits) {
            cout << "Ошибка: введите целое число без букв и других символов.\n";
            continue;
        }

        try {
            int val = stoi(line);
            if (val < low || val > high) {
                cout << "Ошибка: число должно быть в диапазоне от " << low << " до " << high << ".\n";
                continue;
            }
            return val;
        } catch (const exception&) {
            cout << "Ошибка: введите корректное целое число.\n";
        }
    }
}


int main() {
    PayrollDepartment dept;

    while (true) {
        cout << "\n===== МЕНЮ ОТДЕЛА РАСЧЁТА ЗАРПЛАТЫ =====\n";
        cout << "1. Добавить тип работ\n";
        cout << "2. Показать все типы работ\n";
        cout << "3. Вычислить среднюю величину оплаты\n";
        cout << "0. Выход\n";
        cout << "========================================\n";

        int choice = inputMenuChoice("Ваш выбор: ", 0, 3);        try {
            if (choice == 0) {
                cout << "Выход из программы.\n";
                break;
            }
            else if (choice == 1) {
                string name = inputNonEmptyString("Введите название типа работ: ");
                double basePay = inputPositiveDouble("Введите базовую оплату: ");
                double bonusPercent = inputNonNegativeDouble("Введите надбавку в процентах (0 если нет): ");

                dept.addWorkType(name, basePay, bonusPercent);
                cout << "Тип работ успешно добавлен.\n";

            }
            else if (choice == 2) {
                dept.printAll();
            }
            else if (choice == 3) {
                double avg = dept.calculateAveragePay();
                cout << fixed << setprecision(2);
                cout << "Средняя величина оплаты: " << avg << '\n';
            }
        }
        catch (const PayrollException& ex) {
            cout << "Ошибка расчёта зарплаты: " << ex.what() << '\n';
        }
        catch (const exception& ex) {
            cout << "Непредвиденная ошибка: " << ex.what() << '\n';
        }
    }

    return 0;
}
