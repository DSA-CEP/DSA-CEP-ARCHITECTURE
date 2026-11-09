#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
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

    // Searches every line and displays the line and column of each match.
    int searchText(const string &searchTerm)
    {
        if (searchTerm.empty())
        {
            return 0;
        }

        int matchCount = 0;
        int lineNumber = 1;
        Node *current = head;

        while (current != nullptr)
        {
            size_t position = current->data.find(searchTerm);

            while (position != string::npos)
            {
                cout << "Line " << lineNumber
                     << ", Column " << position + 1 << endl;

                matchCount++;

                // Move one character forward to also find overlapping matches.
                position = current->data.find(searchTerm, position + 1);
            }

            current = current->next;
            lineNumber++;
        }

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
        cout << "\nEnter filename (example: notes.txt): ";
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

    DWORD written = 0;
    // Clear each visible row independently so a non-zero window offset
    // cannot make the clearing operation spill into the wrong rows.
    for (int row = 0; row < height; row++)
    {
        COORD rowStart = {(SHORT)left, (SHORT)(top + row)};
        FillConsoleOutputCharacterA(console, ' ', (DWORD)width,
                                    rowStart, &written);
        FillConsoleOutputAttribute(console, info.wAttributes,
                                   (DWORD)width, rowStart, &written);
    }

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
        if (lineIndex >= totalLines) break;

        Node *current = document.getLineAt(lineIndex);
        string number = to_string(lineIndex + 1);
        string line = " " + string(lineNumberWidth - number.length(), ' ') +
                      number + " | " + current->data;
        writeConsoleRow(console, (short)left,
                        (short)(top + headerRows + row), line, width);
    }

    int footerTop = top + height - footerRows;
    writeConsoleRow(console, (short)left, (short)footerTop, string(width, '-'), width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 1),
                    " Ctrl+S Save | Ctrl+A Save As | Ctrl+O Open | Ctrl+Space Search", width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 2),
                    " Ctrl+Z Undo | Ctrl+Y Redo | Esc Main Menu", width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 3),
                    status.empty() ? " Arrows Move | Enter New Line | Backspace/Delete Remove" : " " + status, width);

    int cursorX = left + prefixWidth + document.cursorColumn;
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

// ================= TEXT EDITOR =================

void runEditor(Document &document, UndoRedo &history,
               string &filename)
{
    bool running = true;
    string status = "";
    // Save As does not establish the document's primary Save filename.
    // If Save As is used first on an untitled document, Ctrl+S stays unavailable
    // until the document is started with a normal Save workflow.
    bool saveAsUsedBeforeFirstSave = false;

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

        if (key == 224)
        {
            key = _getch();

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
                cout << "\nSAVE AS - enter a new .txt filename\n";
                string newFilename = getFilename(true);

                if (!newFilename.empty())
                {
                    if (FileManager::saveFile(document, newFilename))
                    {
                        if (filename.empty())
                        {
                            saveAsUsedBeforeFirstSave = true;
                        }
                        status = "DOCUMENT SAVED AS " + newFilename;
                    }
                    else status = "ERROR: DOCUMENT COULD NOT BE SAVED";
                }
                break;
            }

            case 0: // Ctrl + Space - Search
            {
                system("cls");

                cout << "========== SEARCH DOCUMENT ==========" << endl;
                cout << "Enter a word or phrase: ";

                string searchTerm;
                getline(cin, searchTerm);

                system("cls");
                cout << "========== SEARCH RESULTS ==========" << endl;

                if (searchTerm.empty())
                {
                    cout << "Search term cannot be empty." << endl;
                }
                else
                {
                    cout << "Search term: " << searchTerm << endl
                         << endl;

                    int matches = document.searchText(searchTerm);

                    if (matches == 0)
                    {
                        cout << "No matches found." << endl;
                    }
                    else
                    {
                        cout << endl
                             << "Total matches: " << matches << endl;
                    }
                }

                cout << endl
                     << "Press any key to return to the editor...";
                _getch();

                break;
            }

            case 19: // Ctrl + S
            {
                if (filename.empty() && saveAsUsedBeforeFirstSave)
                {
                    status = "ERROR: CTRL+S IS UNAVAILABLE. SAVE AS WAS USED FIRST.";
                }
                else
                {
                    if (filename.empty())
                    {
                        cout << "\nSAVE - enter a filename for this document\n";
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
                }
                break;
            }

            case 15: // Ctrl + O
            {
                system("cls");

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
