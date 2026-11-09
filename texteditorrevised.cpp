#include <iostream>
#include <string>
#include <fstream>
#include <vector>
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

// Gets the filename, automatically adds .txt, and optionally checks uniqueness.
string getFilename(bool requireUnique = false)
{
    string filename;

    while (true)
    {
        cout << "\nEnter filename (without extension): ";
        getline(cin, filename);
        cout << endl;

        if (filename.empty())
        {
            return "";
        }

        // Accept a base name only; avoid adding .txt twice if it was typed.
        if (filename.length() >= 4 &&
            filename.substr(filename.length() - 4) == ".txt")
        {
            filename.erase(filename.length() - 4);
        }

        filename += ".txt";

        string fullPath = FOLDER_NAME + filename;

        // New files and Save As must not overwrite an existing document.
        if (requireUnique &&
            GetFileAttributesA(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            cout << "ERROR: FILE NAME ALREADY USED. PLEASE ENTER A UNIQUE NAME."
                 << endl;
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
            showMessage("ERROR: COULD NOT OPEN FILE FOR WRITING!");
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
            showMessage("ERROR: FILE COULD NOT BE SAVED!");
            return false;
        }

        showMessage("DOCUMENT SAVED SUCCESSFULLY!");
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

void displayEditor(Document &document, string filename)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(console, &info);

    DWORD screenSize = info.dwSize.X * info.dwSize.Y;
    DWORD written;

    COORD start = {0, 0};

    FillConsoleOutputCharacter(
        console, ' ', screenSize, start, &written);

    FillConsoleOutputAttribute(
        console, info.wAttributes, screenSize, start, &written);

    SetConsoleCursorPosition(console, start);

    int lineNumberWidth = 1;
    int totalLines = document.getLineCount();

    while (totalLines >= 10)
    {
        lineNumberWidth++;
        totalLines /= 10;
    }

    int cursorLine = document.getCursorLineIndex();

    cout << "+==============================================================+" << endl;
    cout << "|                         TEXT EDITOR                          |" << endl;
    cout << "+==============================================================+" << endl;

    cout << " File: ";
    if (filename.empty())
    {
        cout << "Untitled";
    }
    else
    {
        cout << filename;
    }
    cout << endl;

    cout << " Current Line: " << cursorLine + 1
         << " | Column: " << document.cursorColumn + 1
         << " | Total Lines: " << document.getLineCount()
         << " | Characters: " << document.getCharacterCount() << endl;

    cout << "+--------------------------------------------------------------+" << endl;

    Node *current = document.getLineAt(0);
    int currentRow = 6;

    while (current != nullptr)
    {
        cout << " ";

        int digits = 1;
        int number = currentRow - 5;
        int temp = number;

        while (temp >= 10)
        {
            digits++;
            temp /= 10;
        }

        for (int i = digits; i < lineNumberWidth; i++)
        {
            cout << " ";
        }

        cout << number << " | " << current->data << endl;

        current = current->next;
        currentRow++;
    }

    cout << "+--------------------------------------------------------------+" << endl;
    cout << " Ctrl+S Save | Ctrl+A Save As | Ctrl+O Open | Ctrl+G Search" << endl;
    cout << " Ctrl+Z Undo | Ctrl+Y Redo | Esc Main Menu" << endl;
    cout << " Arrows Move | Enter New Line | Backspace/Delete Remove" << endl;
    cout << "+--------------------------------------------------------------+" << endl;

    cout.flush();

    GetConsoleScreenBufferInfo(console, &info);

    COORD position;

    position.X = lineNumberWidth + 3 + document.cursorColumn;
    position.Y = 6 + cursorLine;

    if (position.X >= info.dwSize.X)
    {
        position.X = info.dwSize.X - 1;
    }

    if (position.Y >= info.dwSize.Y)
    {
        position.Y = info.dwSize.Y - 1;
    }

    SetConsoleCursorPosition(console, position);
}

// ================= TEXT EDITOR =================

void runEditor(Document &document, UndoRedo &history,
               string &filename)
{
    bool running = true;

    while (running)
    {
        displayEditor(document, filename);

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
                system("cls");

                cout << "========== SAVE AS ==========" << endl;
                string newFilename = getFilename(true);

                if (!newFilename.empty())
                {
                    if (FileManager::saveFile(document, newFilename))
                    {
                        filename = newFilename;
                    }
                }

                break;
            }

            case 7: // Ctrl + G - Search
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
                if (filename.empty())
                {
                    filename = getFilename(true);
                }

                if (!filename.empty())
                {
                    FileManager::saveFile(document, filename);
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