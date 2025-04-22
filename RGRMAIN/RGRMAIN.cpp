#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cctype>

using namespace std;

const int SIZE = 101;

struct Entry {
    string key;
    vector<int> indices;
    bool isDeleted;

    Entry(const string& k, int index) : key(k), isDeleted(false) {
        indices.push_back(index);
    }
};

vector<int> BoyerMooreMatch(const string& haystack, const string& needle) {
    vector<int> foundPositions;
    int textLength = haystack.size();
    int patternLength = needle.size();

    if (patternLength == 0 || patternLength > textLength)
        return foundPositions;

    unordered_map<char, int> badCharShift;
    for (int i = 0; i < patternLength; ++i)
        badCharShift[needle[i]] = i;

    int shift = 0;

    while (shift <= textLength - patternLength) {
        int j = patternLength - 1;
        while (j >= 0 && needle[j] == haystack[shift + j])
            j--;

        if (j < 0) {
            foundPositions.push_back(shift);
            shift += (shift + patternLength < textLength) ? patternLength - badCharShift[haystack[shift + patternLength]] : 1;
        }
        else {
            int offset = j - (badCharShift.count(haystack[shift + j]) ? badCharShift[haystack[shift + j]] : -1);
            shift += max(1, offset);
        }
    }

    return foundPositions;
}

vector<pair<string, int>> extractWordsWithOffsets(const string& raw) {
    vector<pair<string, int>> extracted;
    int i = 0;
    while (i < raw.length()) {
        while (i < raw.length() && !isalpha(raw[i])) ++i;
        int begin = i;
        while (i < raw.length() && isalpha(raw[i])) ++i;
        if (begin < i)
            extracted.emplace_back(raw.substr(begin, i - begin), begin);
    }
    return extracted;
}

class TextIndexer {
private:
    vector<Entry*> storage;

    int computeHash(const string& word) {
        unsigned int result = 0;
        for (char ch : word)
            result = 31 * result + ch;
        return result % SIZE;
    }

    bool isEqual(const string& a, const string& b) {
        return !BoyerMooreMatch(a, b).empty() && a.length() == b.length();
    }

public:
    TextIndexer() {
        storage.resize(SIZE, nullptr);
    }

    ~TextIndexer() {
        for (auto item : storage)
            delete item;
    }

    void add(const string& word, int position) {
        int index = computeHash(word);
        int startIndex = index;

        while (storage[index] != nullptr && !isEqual(storage[index]->key, word)) {
            index = (index + 1) % SIZE;
            if (index == startIndex) return;
        }

        if (storage[index] == nullptr || storage[index]->isDeleted) {
            delete storage[index];
            storage[index] = new Entry(word, position);
        }
        else {
            storage[index]->indices.push_back(position);
        }
    }

    bool erase(const string& word) {
        int index = computeHash(word);
        int startIndex = index;

        while (storage[index] != nullptr) {
            if (!storage[index]->isDeleted && isEqual(storage[index]->key, word)) {
                storage[index]->isDeleted = true;
                return true;
            }
            index = (index + 1) % SIZE;
            if (index == startIndex) break;
        }

        return false;
    }

    vector<int> locate(const string& word) {
        int index = computeHash(word);
        int startIndex = index;

        while (storage[index] != nullptr) {
            if (!storage[index]->isDeleted && isEqual(storage[index]->key, word))
                return storage[index]->indices;
            index = (index + 1) % SIZE;
            if (index == startIndex) break;
        }

        return {};
    }

    void display() {
        cout << "\n";
        for (auto entry : storage) {
            if (entry && !entry->isDeleted) {
                cout << entry->key << ": ";
                for (int i : entry->indices)
                    cout << i << " ";
                cout << endl;
            }
        }
    }

    void saveToDisk(const string& filename) {
        ofstream file(filename);
        for (auto entry : storage) {
            if (entry && !entry->isDeleted) {
                file << entry->key << ": ";
                for (int i : entry->indices)
                    file << i << " ";
                file << "\n";
            }
        }
    }
};

int main() {
    setlocale(LC_ALL, "");

    ifstream inputFile("text.txt");
    ifstream dictFile("words.txt");

    if (!inputFile || !dictFile) {
        cout << "Ошибка открытия файлов.\n";
        return 1;
    }

    string document((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
    vector<string> dictionary;
    string current;

    while (dictFile >> current)
        dictionary.push_back(current);

    TextIndexer hashtable;
    vector<pair<string, int>> parsedWords = extractWordsWithOffsets(document);

    for (const string& word : dictionary) {
        for (const auto& entry : parsedWords) {
            if (entry.first == word)
                hashtable.add(word, entry.second);
        }
    }

    int action = -1;
    string query;

    do {
        cout << "\n";
        cout << "1. Поиск слова\n";
        cout << "2. Добавить слово\n";
        cout << "3. Удалить слово\n";
        cout << "4. Показать индекс\n";
        cout << "\nlog: ";
        cin >> action;

        switch (action) {
        case 1:
            cout << "Введите слово: ";
            cin >> query;
            {
                vector<int> found = hashtable.locate(query);
                if (found.empty()) {
                    cout << "-1\n";
                }
                else {
                    cout << "Позиции: ";
                    for (int pos : found)
                        cout << pos << " ";
                    cout << endl;
                }
            }
            break;

        case 2:
            cout << "Введите слово для добавления: ";
            cin >> query;
            {
                if (!document.empty() && !isspace(document.back()))
                    document += " ";
                int location = document.size();
                document += query;
                hashtable.add(query, location);

                ofstream updateFile("text.txt");
                updateFile << document;
                updateFile.close();

                cout << "Слово добавлено в текст на позицию " << location << ".\n";
            }
            break;

        case 3:
            cout << "Введите слово для удаления: ";
            cin >> query;
            if (hashtable.erase(query))
                cout << "Слово удалено.\n";
            else
                cout << "Слово не найдено.\n";
            break;

        case 4:
            hashtable.display();
            break;

        case 0:
            hashtable.saveToDisk("table.txt");
            cout << "Завершение программы.\n";
            break;

        default:
            cout << "Неверный выбор. Попробуйте снова.\n";
            break;
        }

    } while (action != 0);

    return 0;
}
