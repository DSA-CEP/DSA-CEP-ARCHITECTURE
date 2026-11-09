#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <conio.h>
#include <windows.h>
using namespace std;

// Action types
const int INSERT_CHAR = 1;
const int DELETE_CHAR = 2;
const int SPLIT_LINE = 3;
const int JOIN_LINES = 4;

// Displays a message and waits for a key.
void showMessage(string message)
{
    system("cls");
    cout << message << endl;
    cout << "\nPress any key to continue...";
    _getch();
}

// Action class stores information about an editing operation.
class Action
{
public:
    int type;
    int line;
    int column;
    string text;

    int beforeLine;
    int beforeColumn;

    int afterLine;
    int afterColumn;

    Action()
    {
        type = 0;
        line = 0;
        column = 0;
        beforeLine = 0;
        beforeColumn = 0;
        afterLine = 0;
        afterColumn = 0;
    }

    Action(int t, int l, int c, string txt,
           int bl, int bc, int al, int ac)
    {
        type = t;
        line = l;
        column = c;
        text = txt;
        beforeLine = bl;
        beforeColumn = bc;
        afterLine = al;
        afterColumn = ac;
    }
};

// Template-based Stack.
template <class T>
class Stack
{
private:
    T *arr;
    int top;
    int capacity;

public:
    Stack()
    {
        capacity = 10;
        top = -1;
        arr = new T[capacity];
    }

    void push(T value)
    {
        if (top == capacity - 1)
        {
            capacity *= 2;
            T *newArr = new T[capacity];

            for (int i = 0; i <= top; i++)
            {
                newArr[i] = arr[i];
            }

            delete[] arr;
            arr = newArr;
        }

        arr[++top] = value;
    }

    bool pop(T &value)
    {
        if (isEmpty())
        {
            return false;
        }

        value = arr[top--];
        return true;
    }

    T *peek()
    {
        if (isEmpty())
        {
            return nullptr;
        }

        return &arr[top];
    }

    bool isEmpty()
    {
        return top == -1;
    }

    void clear()
    {
        top = -1;
    }

    ~Stack()
    {
        delete[] arr;
    }
};

// Each node stores one line.
class Node
{
public:
    string data;
    Node *next;
    Node *prev;

    Node(string text)
    {
        data = text;
        next = nullptr;
        prev = nullptr;
    }
};

// Document management using a doubly linked list.
class Document
{
private:
    Node *head;
    Node *tail;
    int lineCount;

public:
    Node *cursorLine;
    int cursorColumn;

    Document()
    {
        head = new Node("");
        tail = head;
        cursorLine = head;
        cursorColumn = 0;
        lineCount = 1;
    }

    int getLineCount()
    {
        return lineCount;
    }

    // Counts all characters in the document, excluding newline characters.
    int getCharacterCount()
    {
        int total = 0;
        Node *current = head;

        while (current != nullptr)
        {
            total += current->data.length();
            current = current->next;
        }

        return total;
    }

    // Case-insensitive search across every document line. Moves the cursor to
    // the first match so the editor viewport can jump directly to that location.
    int searchText(const string &searchTerm)
    {
        if (searchTerm.empty())
            return 0;

        string foldedTerm = searchTerm;
        transform(foldedTerm.begin(), foldedTerm.end(), foldedTerm.begin(),
                  [](unsigned char ch) { return static_cast<char>(tolower(ch)); });

        int matchCount = 0;
        int lineNumber = 0;
        int firstMatchLine = -1;
        int firstMatchColumn = -1;
        Node *current = head;

        while (current != nullptr)
        {
            string foldedLine = current->data;
            transform(foldedLine.begin(), foldedLine.end(), foldedLine.begin(),
                      [](unsigned char ch) { return static_cast<char>(tolower(ch)); });

            size_t position = foldedLine.find(foldedTerm);
            while (position != string::npos)
            {
                if (firstMatchLine == -1)
                {
                    firstMatchLine = lineNumber;
                    firstMatchColumn = static_cast<int>(position);
                }

                ++matchCount;
                position = foldedLine.find(foldedTerm, position + 1);
            }

            current = current->next;
            ++lineNumber;
        }

        if (firstMatchLine >= 0)
            setCursor(firstMatchLine, firstMatchColumn);

        return matchCount;
    }

    Node *getLineAt(int index)
    {
        if (index < 0 || index >= lineCount)
        {
            return nullptr;
        }

        Node *current;

        if (index <= lineCount / 2)
        {
            current = head;

            for (int i = 0; i < index; i++)
            {
                current = current->next;
            }
        }
        else
        {
            current = tail;

            for (int i = lineCount - 1; i > index; i--)
            {
                current = current->prev;
            }
        }

        return current;
    }

    int getCursorLineIndex()
    {
        int index = 0;
        Node *current = head;

        while (current != cursorLine && current != nullptr)
        {
            current = current->next;
            index++;
        }

        return index;
    }

    void setCursor(int line, int column)
    {
        Node *target = getLineAt(line);

        if (target == nullptr)
        {
            return;
        }

        cursorLine = target;

        if (column > target->data.length())
        {
            column = target->data.length();
        }

        if (column < 0)
        {
            column = 0;
        }

        cursorColumn = column;
    }

    void insertCharacterAt(int line, int column, char ch)
    {
        Node *target = getLineAt(line);

        if (target != nullptr && column >= 0 &&
            column <= target->data.length())
        {
            target->data.insert(column, 1, ch);
        }
    }

    void deleteCharacterAt(int line, int column)
    {
        Node *target = getLineAt(line);

        if (target != nullptr && column >= 0 &&
            column < target->data.length())
        {
            target->data.erase(column, 1);
        }
    }

    void splitLineAt(int line, int column)
    {
        Node *current = getLineAt(line);

        if (current == nullptr)
        {
            return;
        }

        if (column < 0)
        {
            column = 0;
        }

        if (column > current->data.length())
        {
            column = current->data.length();
        }

        string remaining = current->data.substr(column);
        current->data.erase(column);

        Node *newNode = new Node(remaining);

        newNode->next = current->next;
        newNode->prev = current;

        if (current->next != nullptr)
        {
            current->next->prev = newNode;
        }
        else
        {
            tail = newNode;
        }

        current->next = newNode;
        lineCount++;
    }

    void joinLineAt(int line, int column)
    {
        Node *current = getLineAt(line);

        if (current == nullptr || current->next == nullptr)
        {
            return;
        }

        Node *nextNode = current->next;

        current->data += nextNode->data;
        current->next = nextNode->next;

        if (nextNode->next != nullptr)
        {
            nextNode->next->prev = current;
        }
        else
        {
            tail = current;
        }

        delete nextNode;
        lineCount--;
    }

    // Replace document contents with lines loaded from a file.
    void replaceLines(vector<string> &lines)
    {
        Node *current = head;

        while (current != nullptr)
        {
            Node *temp = current;
            current = current->next;
            delete temp;
        }

        head = nullptr;
        tail = nullptr;
        lineCount = 0;

        if (lines.empty())
        {
            head = new Node("");
            tail = head;
            lineCount = 1;
        }
        else
        {
            for (int i = 0; i < lines.size(); i++)
            {
                Node *newNode = new Node(lines[i]);

                if (head == nullptr)
                {
                    head = newNode;
                    tail = newNode;
                }
                else
                {
                    tail->next = newNode;
                    newNode->prev = tail;
                    tail = newNode;
                }

                lineCount++;
            }
        }

        cursorLine = head;
        cursorColumn = 0;
    }

    void display()
    {
        Node *current = head;

        while (current != nullptr)
        {
            cout << current->data << endl;
            current = current->next;
        }
    }

    ~Document()
    {
        Node *current = head;

        while (current != nullptr)
        {
            Node *temp = current;
            current = current->next;
            delete temp;
        }
    }
};

// Manages Undo and Redo.
class UndoRedo
{
private:
    Stack<Action> undoStack;
    Stack<Action> redoStack;

public:
    void record(Action action)
    {
        undoStack.push(action);
        redoStack.clear();
    }

    void clearHistory()
    {
        undoStack.clear();
        redoStack.clear();
    }

    void undo(Document &document)
    {
        Action action;

        if (!undoStack.pop(action))
        {
            return;
        }

        switch (action.type)
        {
        case INSERT_CHAR:
            document.deleteCharacterAt(
                action.line, action.column);
            break;

        case DELETE_CHAR:
            document.insertCharacterAt(
                action.line, action.column, action.text[0]);
            break;

        case SPLIT_LINE:
            document.joinLineAt(
                action.line, action.column);
            break;

        case JOIN_LINES:
            document.splitLineAt(
                action.line, action.column);
            break;
        }

        document.setCursor(
            action.beforeLine, action.beforeColumn);

        redoStack.push(action);
    }

    void redo(Document &document)
    {
        Action action;

        if (!redoStack.pop(action))
        {
            return;
        }

        switch (action.type)
        {
        case INSERT_CHAR:
            document.insertCharacterAt(
                action.line, action.column, action.text[0]);
            break;

        case DELETE_CHAR:
            document.deleteCharacterAt(
                action.line, action.column);
            break;

        case SPLIT_LINE:
            document.splitLineAt(
                action.line, action.column);
            break;

        case JOIN_LINES:
            document.joinLineAt(
                action.line, action.column);
            break;
        }

        document.setCursor(
            action.afterLine, action.afterColumn);

        undoStack.push(action);
    }
};

// ================= FILE HANDLING =================

const string FOLDER_NAME = "TextEditorFiles\\";

// Creates the folder if it does not already exist.
void createDocumentFolder()
{
    CreateDirectoryA("TextEditorFiles", NULL);
}

// Gets a filename and validates that the user explicitly enters .txt.
// requireUnique is used for new documents and Save As.
string getFilename(bool requireUnique = false)
{
    string filename;

    while (true)
    {
        // Use a clean prompt screen so the filename input never overlaps
        // the editor's footer or the current editing cursor.
        system("cls");
        cout << "Enter filename (example: notes.txt): ";
        getline(cin, filename);

        if (filename.empty())
            return "";

        // Only .txt is supported. Do not silently change another extension.
        if (filename.length() <= 4 ||
            filename.substr(filename.length() - 4) != ".txt")
        {
            cout << "ERROR: INVALID FILE EXTENSION. ONLY .txt FILES ARE ALLOWED.\n";
            cout << "Press any key to try again...";
            _getch();
            cout << "\n";
            continue;
        }

        // Reject paths and Windows-invalid filename characters.
        if (filename.find_first_of("\\/:*?\"<>|") != string::npos)
        {
            cout << "ERROR: INVALID FILENAME. ENTER A NAME, NOT A PATH.\n";
            cout << "Press any key to try again...";
            _getch();
            cout << "\n";
            continue;
        }

        string fullPath = FOLDER_NAME + filename;

        if (requireUnique &&
            GetFileAttributesA(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            cout << "ERROR: FILE NAME ALREADY USED. PLEASE ENTER A UNIQUE NAME.\n";
            cout << "Press any key to try again...";
            _getch();
            cout << "\n";
            continue;
        }

        return fullPath;
    }
}

// Handles saving and opening text documents.
class FileManager
{
public:
    // Save document as a normal readable text file.
    static bool saveFile(Document &document, string filename)
    {
        ofstream file(filename);

        if (!file)
        {
            return false;
        }

        Node *current = document.getLineAt(0);

        while (current != nullptr)
        {
            file << current->data << '\n';
            current = current->next;
        }

        file.close();

        if (file.fail())
        {
            return false;
        }

        return true;
    }

    // Open a normal readable text file.
    static bool openFile(Document &document, string filename)
    {
        ifstream file(filename);

        if (!file)
        {
            showMessage("ERROR: FILE NOT FOUND!");
            return false;
        }

        vector<string> lines;
        string line;

        while (getline(file, line))
        {
            lines.push_back(line);
        }

        if (file.bad())
        {
            showMessage("ERROR: COULD NOT READ FILE!");
            return false;
        }

        file.close();

        document.replaceLines(lines);

        showMessage("DOCUMENT OPENED SUCCESSFULLY!");
        return true;
    }
};

// ================= EDITOR DISPLAY =================

void writeConsoleRow(HANDLE console, short x, short y, const string &value, int width)
{
    if (width <= 0) return;

    string row = value;
    if ((int)row.length() > width)
        row.resize(width);                 // Never allow a line to wrap.
    if ((int)row.length() < width)
        row.append(width - row.length(), ' ');

    COORD position = {x, y};
    DWORD written = 0;
    WriteConsoleOutputCharacterA(console, row.c_str(), (DWORD)width,
                                 position, &written);
}

void displayEditor(Document &document, string filename, const string &status)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(console, &info);

    // Work inside the visible window, not the full scrollback buffer.
    int width = info.srWindow.Right - info.srWindow.Left + 1;
    int height = info.srWindow.Bottom - info.srWindow.Top + 1;
    int left = info.srWindow.Left;
    int top = info.srWindow.Top;

    if (width < 20 || height < 10)
        return;

    int totalLines = document.getLineCount();
    int lineNumberWidth = 1;
    for (int n = totalLines; n >= 10; n /= 10)
        lineNumberWidth++;

    const int headerRows = 6;
    const int footerRows = 4;
    int contentRows = height - headerRows - footerRows;
    if (contentRows < 1) contentRows = 1;

    int cursorLine = document.getCursorLineIndex();
    static int firstVisibleLine = 0;
    if (cursorLine < firstVisibleLine)
        firstVisibleLine = cursorLine;
    if (cursorLine >= firstVisibleLine + contentRows)
        firstVisibleLine = cursorLine - contentRows + 1;
    if (firstVisibleLine > totalLines - contentRows)
        firstVisibleLine = max(0, totalLines - contentRows);

    string border(width, '=');
    writeConsoleRow(console, (short)left, (short)top, border, width);
    string heading = "TEXT EDITOR";
    int headingStart = max(0, (width - (int)heading.length()) / 2);
    string headingRow(width, ' ');
    headingRow.replace(headingStart, heading.length(), heading);
    writeConsoleRow(console, (short)left, (short)(top + 1), headingRow, width);
    writeConsoleRow(console, (short)left, (short)(top + 2), border, width);

    string fileRow = " File: " + (filename.empty() ? "Untitled" : filename);
    writeConsoleRow(console, (short)left, (short)(top + 3), fileRow, width);

    string infoRow = " Current Line: " + to_string(cursorLine + 1) +
        " | Column: " + to_string(document.cursorColumn + 1) +
        " | Total Lines: " + to_string(totalLines) +
        " | Characters: " + to_string(document.getCharacterCount());
    writeConsoleRow(console, (short)left, (short)(top + 4), infoRow, width);
    writeConsoleRow(console, (short)left, (short)(top + 5), string(width, '-'), width);

    int prefixWidth = 1 + lineNumberWidth + 3; // leading space, number, " | "
    for (int row = 0; row < contentRows; row++)
    {
        int lineIndex = firstVisibleLine + row;

        // Always rewrite each document row. Empty rows are cleared by
        // writeConsoleRow's padding, without clearing the whole terminal.
        if (lineIndex >= totalLines)
        {
            writeConsoleRow(console, (short)left,
                            (short)(top + headerRows + row), "", width);
            continue;
        }

        Node *current = document.getLineAt(lineIndex);
        string number = to_string(lineIndex + 1);
        string linePrefix = " " + string(lineNumberWidth - number.length(), ' ') +
                            number + " | ";
        string visibleText = current->data;

        // If the cursor is far across a long line, horizontally scroll that
        // line so the actual cursor/search match remains visible.
        if (lineIndex == cursorLine)
        {
            int textWidth = max(1, width - prefixWidth);
            int horizontalOffset = max(0, document.cursorColumn - textWidth + 1);
            if (horizontalOffset > static_cast<int>(visibleText.length()))
                horizontalOffset = static_cast<int>(visibleText.length());
            visibleText = visibleText.substr(horizontalOffset);
        }

        writeConsoleRow(console, (short)left,
                        (short)(top + headerRows + row), linePrefix + visibleText, width);
    }

    int footerTop = top + height - footerRows;
    writeConsoleRow(console, (short)left, (short)footerTop, string(width, '-'), width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 1),
                    " Ctrl+S Save | Ctrl+A Save As | Ctrl+O Open | Ctrl+Space Search", width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 2),
                    " Ctrl+Z Undo | Ctrl+Y Redo | Esc Main Menu", width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 3),
                    status.empty() ? " Arrows Move | Enter New Line | Backspace/Delete Remove" : " " + status, width);

    Node *activeLine = document.getLineAt(cursorLine);
    int textWidth = max(1, width - prefixWidth);
    int horizontalOffset = max(0, document.cursorColumn - textWidth + 1);
    if (activeLine != nullptr && horizontalOffset > static_cast<int>(activeLine->data.length()))
        horizontalOffset = static_cast<int>(activeLine->data.length());

    int cursorX = left + prefixWidth + document.cursorColumn - horizontalOffset;
    int cursorY = top + headerRows + (cursorLine - firstVisibleLine);
    int maxX = info.srWindow.Right;
    int maxY = top + headerRows + contentRows - 1;
    if (cursorX > maxX) cursorX = maxX;
    if (cursorX < left) cursorX = left;
    if (cursorY > maxY) cursorY = maxY;
    if (cursorY < top + headerRows) cursorY = top + headerRows;

    COORD cursorPosition = {(SHORT)cursorX, (SHORT)cursorY};
    SetConsoleCursorPosition(console, cursorPosition);
}

// Every occurrence of the current search term, stored as zero-based line/column.
struct SearchMatch
{
    int line;
    int column;
};

static vector<SearchMatch> gSearchMatches;
static int gCurrentSearchMatch = -1;
static string gCurrentSearchTerm;

// Search helper shared by Ctrl+Space and Ctrl+F2.
void searchDocument(Document &document, string &status)
{
    system("cls");
    cout << "========== SEARCH DOCUMENT ==========" << endl;
    cout << "Enter a word or phrase: ";

    string searchTerm;
    getline(cin, searchTerm);

    if (searchTerm.empty())
    {
        status = "SEARCH CANCELLED - EMPTY SEARCH TERM";
        return;
    }

    gSearchMatches.clear();
    gCurrentSearchMatch = -1;
    gCurrentSearchTerm = searchTerm;

    string foldedTerm = searchTerm;
    transform(foldedTerm.begin(), foldedTerm.end(), foldedTerm.begin(),
              [](unsigned char ch) { return static_cast<char>(tolower(ch)); });

    int lineNumber = 0;
    Node *current = document.getLineAt(0);

    while (current != nullptr)
    {
        string foldedLine = current->data;
        transform(foldedLine.begin(), foldedLine.end(), foldedLine.begin(),
                  [](unsigned char ch) { return static_cast<char>(tolower(ch)); });

        size_t position = foldedLine.find(foldedTerm);
        while (position != string::npos)
        {
            SearchMatch match;
            match.line = lineNumber;
            match.column = static_cast<int>(position);
            gSearchMatches.push_back(match);

            // Advance by one to count overlapping occurrences too.
            position = foldedLine.find(foldedTerm, position + 1);
        }

        current = current->next;
        ++lineNumber;
    }

    if (gSearchMatches.empty())
    {
        status = "NO MATCHES FOUND: " + searchTerm;
        return;
    }

    gCurrentSearchMatch = 0;
    document.setCursor(gSearchMatches[0].line, gSearchMatches[0].column);
    status = "FOUND " + to_string(gSearchMatches.size()) + " MATCHES - MATCH 1 OF " +
             to_string(gSearchMatches.size()) + ": " + searchTerm +
             " (PRESS F3 FOR NEXT)";
}

void goToNextSearchMatch(Document &document, string &status)
{
    if (gSearchMatches.empty() || gCurrentSearchMatch < 0)
    {
        status = "SEARCH FIRST USING CTRL+F2";
        return;
    }

    gCurrentSearchMatch = (gCurrentSearchMatch + 1) %
                          static_cast<int>(gSearchMatches.size());

    const SearchMatch &match = gSearchMatches[gCurrentSearchMatch];
    document.setCursor(match.line, match.column);

    status = "MATCH " + to_string(gCurrentSearchMatch + 1) + " OF " +
             to_string(gSearchMatches.size()) + ": " + gCurrentSearchTerm +
             " (PRESS F3 FOR NEXT)";
}

// ================= TEXT EDITOR =================

void runEditor(Document &document, UndoRedo &history,
               string &filename)
{
    bool running = true;
    string status = "";

    while (running)
    {
        displayEditor(document, filename, status);

        int key = _getch();

        // ESC returns to the main menu.
        if (key == 27)
        {
            running = false;
            break;
        }

        int line = document.getCursorLineIndex();
        int column = document.cursorColumn;

        if (key == 0 || key == 224)
        {
            key = _getch();

            // Ctrl+Space can be delivered as NUL followed by ASCII space.
            if (key == 32)
            {
                searchDocument(document, status);
                continue;
            }

            // F3 moves to the next occurrence of the active search.
            if (key == 61)
            {
                goToNextSearchMatch(document, status);
                continue;
            }

            // MSVC _getch() reports Ctrl+F2 as an extended key:
            // prefix 0/224 followed by scan code 0x5F.
            // 0x5E is Ctrl+F1, not Ctrl+F2.
            if (key == 0x5F)
            {
                searchDocument(document, status);
                continue;
            }

            switch (key)
            {
            case 72: // Up
                if (line > 0)
                {
                    document.setCursor(line - 1, column);
                }
                break;

            case 80: // Down
                if (document.getLineAt(line)->next != nullptr)
                {
                    document.setCursor(line + 1, column);
                }
                break;

            case 75: // Left
                if (column > 0)
                {
                    document.setCursor(line, column - 1);
                }
                else if (line > 0)
                {
                    Node *previous = document.getLineAt(line - 1);

                    document.setCursor(
                        line - 1,
                        previous->data.length());
                }
                break;

            case 77: // Right
                if (column < document.getLineAt(line)->data.length())
                {
                    document.setCursor(line, column + 1);
                }
                else if (document.getLineAt(line)->next != nullptr)
                {
                    document.setCursor(line + 1, 0);
                }
                break;

            case 71: // Home
                document.setCursor(line, 0);
                break;

            case 79: // End
                document.setCursor(
                    line,
                    document.getLineAt(line)->data.length());
                break;

            case 83: // Delete
            {
                Node *current = document.getLineAt(line);

                if (column < current->data.length())
                {
                    string removed(1, current->data[column]);

                    Action action(
                        DELETE_CHAR, line, column, removed,
                        line, column, line, column);

                    history.record(action);

                    document.deleteCharacterAt(line, column);
                }
                else if (current->next != nullptr)
                {
                    Action action(
                        JOIN_LINES, line, column, "",
                        line, column, line, column);

                    history.record(action);

                    document.joinLineAt(line, column);
                }

                break;
            }
            }
        }
        else
        {
            switch (key)
            {
            case 1: // Ctrl + A - Save As
            {
                // Save As must always let the user choose a different name,
                // even if the document has already been saved before.
                string newFilename = getFilename(false);

                if (!newFilename.empty())
                {
                    bool targetExists =
                        GetFileAttributesA(newFilename.c_str()) != INVALID_FILE_ATTRIBUTES;

                    // If this is a different existing file, ask before replacing it.
                    if (targetExists && newFilename != filename)
                    {
                        system("cls");
                        cout << "The file already exists:\n" << newFilename << "\n\n";
                        cout << "Replace this file? (Y/N): ";
                        int answer = _getch();
                        cout << static_cast<char>(answer) << "\n";

                        if (answer != 'y' && answer != 'Y')
                        {
                            status = "SAVE AS CANCELLED - ORIGINAL FILE KEPT";
                            break;
                        }
                    }

                    if (FileManager::saveFile(document, newFilename))
                    {
                        filename = newFilename;
                        status = "DOCUMENT SAVED AS " + newFilename;
                    }
                    else
                    {
                        status = "ERROR: DOCUMENT COULD NOT BE SAVED";
                    }
                }
                else
                {
                    status = "SAVE AS CANCELLED";
                }
                break;
            }

            case 0: // Ctrl + Space - Search
            {
                searchDocument(document, status);
                break;
            }

            case 19: // Ctrl + S
            {
                if (filename.empty())
                {
                    string newFilename = getFilename(true);
                    if (!newFilename.empty()) filename = newFilename;
                }

                if (!filename.empty())
                {
                    if (FileManager::saveFile(document, filename))
                        status = "DOCUMENT SAVED SUCCESSFULLY";
                    else
                        status = "ERROR: DOCUMENT COULD NOT BE SAVED";
                }
                break;
            }

            case 15: // Ctrl + O
            {
                string newFilename = getFilename();

                if (!newFilename.empty())
                {
                    if (FileManager::openFile(document, newFilename))
                    {
                        filename = newFilename;
                        history.clearHistory();
                        status = "DOCUMENT OPENED SUCCESSFULLY";
                    }
                }

                break;
            }

            case 26: // Ctrl + Z
                history.undo(document);
                break;

            case 25: // Ctrl + Y
                history.redo(document);
                break;

            case 13: // Enter
            {
                Action action(
                    SPLIT_LINE, line, column, "",
                    line, column, line + 1, 0);

                history.record(action);

                document.splitLineAt(line, column);
                document.setCursor(line + 1, 0);

                break;
            }

            case 8: // Backspace
            {
                if (column > 0)
                {
                    Node *current = document.getLineAt(line);

                    string removed(1, current->data[column - 1]);

                    Action action(
                        DELETE_CHAR, line, column - 1, removed,
                        line, column, line, column - 1);

                    history.record(action);

                    document.deleteCharacterAt(line, column - 1);
                    document.setCursor(line, column - 1);
                }
                else if (line > 0)
                {
                    Node *previous = document.getLineAt(line - 1);

                    int previousLength = previous->data.length();

                    Action action(
                        JOIN_LINES, line - 1, previousLength, "",
                        line, column, line - 1, previousLength);

                    history.record(action);

                    document.joinLineAt(line - 1, previousLength);

                    document.setCursor(
                        line - 1, previousLength);
                }

                break;
            }

            default:
                if (key >= 32 && key <= 126)
                {
                    Action action(
                        INSERT_CHAR, line, column,
                        string(1, (char)key),
                        line, column, line, column + 1);

                    history.record(action);

                    document.insertCharacterAt(
                        line, column, (char)key);

                    document.setCursor(line, column + 1);
                }

                break;
            }
        }
    }
}

// ================= MAIN MENU =================

int main()
{
    // system("start powershell -NoExit -Command \"Set-Location -LiteralPath (Get-Location); & '.\\3 text editor.exe'\""); //SET YOUR FILE NAME
    // Create the document folder when the program starts.
    createDocumentFolder();

    bool applicationRunning = true;

    while (applicationRunning)
    {
        system("cls");

        cout << "========== TEXT EDITOR ==========" << endl;
        cout << "1. Create New Document" << endl;
        cout << "2. Open Existing Document" << endl;
        cout << "3. Exit" << endl;
        cout << "=================================" << endl;
        cout << "Enter your choice: ";

        string choice;
        getline(cin, choice);

        if (choice == "1")
        {
            Document document;
            UndoRedo history;

            string filename = "";

            runEditor(document, history, filename);
        }
        else if (choice == "2")
        {
            system("cls");

            string filename = getFilename();

            if (!filename.empty())
            {
                Document document;
                UndoRedo history;

                if (FileManager::openFile(document, filename))
                {
                    history.clearHistory();

                    runEditor(document, history, filename);
                }
            }
        }
        else if (choice == "3")
        {
            applicationRunning = false;
        }
        else
        {
            showMessage("INVALID MENU CHOICE!");
        }
    }

    system("cls");
    cout << "Thank you for using the Text Editor!" << endl;

    return 0;
}