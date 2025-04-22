#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cctype>

using namespace std;

const int HASH_CAPACITY = 101;

struct Entry {
    string keyword;
    vector<int> locations;
    Entry(const string& w, int pos) : keyword(w) {
        locations.push_back(pos);
    }
};

class WordIndexTable {
private:
    vector<list<Entry*>> buckets;
    string fullText;

    int computeHash(const string& key) {
        unsigned int h = 0;
        for (char ch : key)
            h = h * 31 + ch;
        return h % HASH_CAPACITY;
    }

    vector<int> boyerMoore(const string& pattern) {
        vector<int> foundPositions;
        int n = fullText.size();
        int m = pattern.size();

        if (m == 0 || n < m) return foundPositions;

        unordered_map<char, int> badChar;
        for (int i = 0; i < m; ++i)
            badChar[pattern[i]] = i;

        int shift = 0;
        while (shift <= n - m) {
            int j = m - 1;
            while (j >= 0 && tolower(pattern[j]) == tolower(fullText[shift + j]))
                --j;

            if (j < 0) {
                if ((shift == 0 || !isalpha(fullText[shift - 1])) &&
                    (shift + m == n || !isalpha(fullText[shift + m]))) {
                    foundPositions.push_back(shift);
                }
                shift += (shift + m < n) ? m - badChar[fullText[shift + m]] : 1;
            }
            else {
                char mismatch = fullText[shift + j];
                int move = j - (badChar.count(mismatch) ? badChar[mismatch] : -1);
                shift += max(1, move);
            }
        }
        return foundPositions;
    }

public:
    WordIndexTable(const string& text) : fullText(text) {
        buckets.resize(HASH_CAPACITY);
    }

    void insert(const string& word, int pos) {
        int index = computeHash(word);
        for (Entry* e : buckets[index]) {
            if (e->keyword == word) {
                e->locations.push_back(pos);
                return;
            }
        }
        buckets[index].push_back(new Entry(word, pos));
    }

    bool erase(const string& word) {
        int index = computeHash(word);
        for (auto it = buckets[index].begin(); it != buckets[index].end(); ++it) {
            if ((*it)->keyword == word) {
                delete* it;
                buckets[index].erase(it);
                return true;
            }
        }
        return false;
    }

    vector<int> searchBM(const string& word) {
        return boyerMoore(word);
    }

    void showAll() {
        for (const auto& bucket : buckets) {
            for (Entry* e : bucket) {
                cout << e->keyword << ": ";
                for (int loc : e->locations)
                    cout << loc << " ";
                cout << endl;
            }
        }
    }

    void save(const string& filename) {
        ofstream file(filename);
        for (const auto& bucket : buckets) {
            for (Entry* e : bucket) {
                file << e->keyword << ": ";
                for (int loc : e->locations)
                    file << loc << " ";
                file << endl;
            }
        }
    }

    ~WordIndexTable() {
        for (auto& bucket : buckets)
            for (Entry* e : bucket)
                delete e;
    }
};

vector<pair<string, int>> extractWordsWithPos(const string& text) {
    vector<pair<string, int>> output;
    int i = 0;
    while (i < text.size()) {
        while (i < text.size() && !isalpha(text[i])) ++i;
        int start = i;
        while (i < text.size() && isalpha(text[i])) ++i;
        if (start < i) {
            output.emplace_back(text.substr(start, i - start), start);
        }
    }
    return output;
}

int main() {
    setlocale(LC_ALL, "");
    ifstream textInput("text.txt");
    ifstream keywordsInput("words.txt");

    if (!textInput || !keywordsInput) {
        cout << "Ошибка при открытии файлов.\n";
        return 1;
    }

    string content((istreambuf_iterator<char>(textInput)), istreambuf_iterator<char>());
    vector<string> keywordList;
    string word;
    while (keywordsInput >> word) {
        keywordList.push_back(word);
    }

    WordIndexTable dictionary(content);
    vector<pair<string, int>> tokens = extractWordsWithPos(content);

    for (const auto& target : keywordList) {
        for (const auto& token : tokens) {
            if (token.first == target) {
                dictionary.insert(target, token.second);
            }
        }
    }

    int cmd;
    do {
        cout << "\n";
        cout << "1. Поиск слова (Бойер-Мур)\n";
        cout << "2. Добавить слово в текст\n";
        cout << "3. Удалить слово\n";
        cout << "4. Показать таблицу\n";
        cout << "0. Выход\n";
        cout << "log: ";
        cin >> cmd;

        string input;
        switch (cmd) {
        case 1:
            cout << "Введите слово: ";
            cin >> input;
            {
                auto results = dictionary.searchBM(input);
                if (results.empty()) cout << "Слово не найдено (БМ).\n";
                else {
                    cout << "Найдено на позициях (БМ): ";
                    for (int p : results) cout << p << " ";
                    cout << endl;
                }
            }
            break;
        case 2:
            cout << "Введите слово для вставки: ";
            cin >> input;
            if (!content.empty() && !isspace(content.back()))
                content += " ";
            int pos;
            pos = content.size();
            content += input;
            dictionary.insert(input, pos);
            {
                ofstream rewrite("text.txt");
                rewrite << content;
            }
            cout << "Слово добавлено на позицию " << pos << endl;
            break;
        case 3:
            cout << "Введите слово для удаления: ";
            cin >> input;
            if (dictionary.erase(input))
                cout << "Удалено.\n";
            else
                cout << "Не найдено.\n";
            break;
        case 4:
            dictionary.showAll();
            break;
        case 0:
            dictionary.save("table.txt");
            cout << "Сохранено и выход.\n";
            break;
        default:
            cout << "Неверная команда.\n";
        }
    } while (cmd != 0);

    return 0;
}
