
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <conio.h>
#include <windows.h>
#include <algorithm>
using namespace std;

// ================= ACTION TYPES =================

const int INSERT_CHAR = 1;
const int DELETE_CHAR = 2;
const int SPLIT_LINE = 3;
const int JOIN_LINES = 4;

// ================= MESSAGE DISPLAY =================

void showMessage(string message)
{
    system("cls");

    cout << "\n\n";
    cout << "       =====================================\n";
    cout << "                    MESSAGE\n";
    cout << "       =====================================\n\n";
    cout << "       " << message << "\n\n";
    cout << "       Press any key to continue...";

    _getch();
}

// ================= ACTION CLASS =================

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
        text = "";
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

// ================= TEMPLATE STACK =================

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

// ================= LINKED LIST NODE =================

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

// ================= DOCUMENT CLASS =================

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

    int getCharacterCount()
    {
        int total = 0;
        Node *current = head;

        while (current != nullptr)
        {
            total += static_cast<int>(current->data.length());
            current = current->next;
        }

        return total;
    }

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

                position = current->data.find(
                    searchTerm, position + 1);
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

        while (current != nullptr && current != cursorLine)
        {
            current = current->next;
            index++;
        }

        if (current == nullptr)
        {
            return 0;
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

        if (column > static_cast<int>(target->data.length()))
        {
            column = static_cast<int>(target->data.length());
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

        if (target != nullptr &&
            column >= 0 &&
            column <= static_cast<int>(target->data.length()))
        {
            target->data.insert(column, 1, ch);
        }
    }

    void deleteCharacterAt(int line, int column)
    {
        Node *target = getLineAt(line);

        if (target != nullptr &&
            column >= 0 &&
            column < static_cast<int>(target->data.length()))
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

        if (column > static_cast<int>(current->data.length()))
        {
            column = static_cast<int>(current->data.length());
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
            for (size_t i = 0; i < lines.size(); i++)
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

    friend class FileManager;
};

// ================= UNDO / REDO =================

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
            if (!action.text.empty())
            {
                document.insertCharacterAt(
                    action.line, action.column, action.text[0]);
            }
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
            if (!action.text.empty())
            {
                document.insertCharacterAt(
                    action.line, action.column, action.text[0]);
            }
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

void createDocumentFolder()
{
    CreateDirectoryA("TextEditorFiles", NULL);
}

// IMPORTANT:
// Filename prompts use a separate, clean screen.
// This prevents editor text from remaining behind the prompt.

string getFilename(bool requireUnique = false)
{
    string filename;

    while (true)
    {
        system("cls");

        cout << "\n\n";
        cout << "       =====================================\n";
        cout << "                 FILE NAME INPUT\n";
        cout << "       =====================================\n\n";
        cout << "       Enter filename (example: notes.txt)\n";
        cout << "       Press Enter without typing to cancel.\n\n";
        cout << "       Filename: ";

        getline(cin, filename);

        if (filename.empty())
        {
            return "";
        }

        // Reject paths and Windows-invalid filename characters.
        if (filename.find_first_of("\\/:*?\"<>|") != string::npos)
        {
            cout << "\n\n";
            cout << "       Invalid filename. Try again.\n";
            cout << "       Press any key to continue...";

            _getch();
            continue;
        }

        if (filename == "." || filename == "..")
        {
            continue;
        }

        // Add .txt only once.
        if (filename.length() >= 4 &&
            filename.substr(filename.length() - 4) == ".txt")
        {
            filename.erase(filename.length() - 4);
        }

        if (filename.empty())
        {
            continue;
        }

        filename += ".txt";

        string fullPath = FOLDER_NAME + filename;

        if (requireUnique &&
            GetFileAttributesA(fullPath.c_str()) !=
                INVALID_FILE_ATTRIBUTES)
        {
            cout << "\n\n";
            cout << "       ERROR: FILE NAME ALREADY EXISTS.\n";
            cout << "       Please enter a different filename.\n\n";
            cout << "       Press any key to try again...";

            _getch();
            continue;
        }

        return fullPath;
    }
}

// ================= FILE MANAGER =================

class FileManager
{
public:
    static bool saveFile(Document &document, string filename)
    {
        ofstream file(filename.c_str());

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

    static bool openFile(Document &document, string filename)
    {
        ifstream file(filename.c_str());

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
    // FIX 1:
    // Clear the entire screen before redrawing.
    // This prevents old prompts and editor commands from overlapping.
    system("cls");

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(console, &info))
    {
        return;
    }

    int consoleWidth = info.dwSize.X;
    int consoleHeight = info.dwSize.Y;

    int totalLines = document.getLineCount();
    int lineNumberWidth = 1;

    int tempLines = totalLines;

    while (tempLines >= 10)
    {
        lineNumberWidth++;
        tempLines /= 10;
    }

    int cursorLine = document.getCursorLineIndex();

    cout << "+==============================================================+\n";
    cout << "|                         TEXT EDITOR                          |\n";
    cout << "+==============================================================+\n";

    cout << " File: ";

    if (filename.empty())
    {
        cout << "Untitled";
    }
    else
    {
        // Display only the filename, not the full folder path.
        size_t separator = filename.find_last_of("\\/");

        if (separator != string::npos)
        {
            cout << filename.substr(separator + 1);
        }
        else
        {
            cout << filename;
        }
    }

    cout << "\n";

    cout << " Current Line: " << cursorLine + 1
         << " | Column: " << document.cursorColumn + 1
         << " | Total Lines: " << document.getLineCount()
         << " | Characters: " << document.getCharacterCount()
         << "\n";

    cout << "+--------------------------------------------------------------+\n";

    // The first document row is row 6.
    const int documentStartRow = 6;

    Node *current = document.getLineAt(0);
    int currentRow = 0;

    // Print document lines.
    while (current != nullptr)
    {
        cout << " ";

        int number = currentRow + 1;
        int digits = 1;
        int numberTemp = number;

        while (numberTemp >= 10)
        {
            digits++;
            numberTemp /= 10;
        }

        for (int i = digits; i < lineNumberWidth; i++)
        {
            cout << " ";
        }

        cout << number << " | " << current->data << "\n";

        current = current->next;
        currentRow++;
    }

    cout << "+--------------------------------------------------------------+\n";

    // FIX 2:
    // Use separate lines for the keyboard shortcuts.
    cout << "\n";
    cout << "  KEYBOARD SHORTCUTS\n";
    cout << "  -----------------------------------------------\n";
    cout << "  Ctrl+S       Save Document\n";
    cout << "  Ctrl+A       Save As\n";
    cout << "  Ctrl+O       Open Document\n";
    cout << "  Ctrl+Space   Search Text\n";
    cout << "  Ctrl+Z       Undo       | Ctrl+Y Redo\n";
    cout << "  Esc          Return to Main Menu\n";
    cout << "  Arrow Keys   Move Cursor\n";
    cout << "  Enter        Create New Line\n";
    cout << "  Backspace    Delete Previous Character\n";
    cout << "  Delete       Delete Next Character / Join Lines\n";
    cout << "  -----------------------------------------------\n";

    cout.flush();

    // FIX 3:
    // Re-read console information after printing.
    if (!GetConsoleScreenBufferInfo(console, &info))
    {
        return;
    }

    // Calculate the cursor's screen coordinates.
    COORD position;

    // One leading space + line number + " | "
    position.X = lineNumberWidth + 4 + document.cursorColumn;
    position.Y = documentStartRow + cursorLine;

    // Keep the cursor within the console width.
    if (position.X < 0)
    {
        position.X = 0;
    }

    if (position.X >= consoleWidth)
    {
        position.X = consoleWidth - 1;
    }

    // Do not position the cursor outside the console buffer.
    if (position.Y < 0)
    {
        position.Y = 0;
    }

    if (position.Y >= consoleHeight)
    {
        position.Y = consoleHeight - 1;
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
            continue;
        }

        int line = document.getCursorLineIndex();
        int column = document.cursorColumn;

        // Extended keyboard keys.
        if (key == 224 || key == 0)
        {
            key = _getch();

            switch (key)
            {
            case 72: // UP
                if (line > 0)
                {
                    document.setCursor(line - 1, column);
                }
                break;

            case 80: // DOWN
                if (line + 1 < document.getLineCount())
                {
                    document.setCursor(line + 1, column);
                }
                break;

            case 75: // LEFT
                if (column > 0)
                {
                    document.setCursor(line, column - 1);
                }
                else if (line > 0)
                {
                    Node *previous = document.getLineAt(line - 1);

                    document.setCursor(
                        line - 1,
                        static_cast<int>(previous->data.length()));
                }
                break;

            case 77: // RIGHT
            {
                Node *current = document.getLineAt(line);

                if (column < static_cast<int>(current->data.length()))
                {
                    document.setCursor(line, column + 1);
                }
                else if (line + 1 < document.getLineCount())
                {
                    document.setCursor(line + 1, 0);
                }

                break;
            }

            case 71: // HOME
                document.setCursor(line, 0);
                break;

            case 79: // END
            {
                Node *current = document.getLineAt(line);

                document.setCursor(
                    line,
                    static_cast<int>(current->data.length()));

                break;
            }

            case 83: // DELETE
            {
                Node *current = document.getLineAt(line);

                if (column < static_cast<int>(current->data.length()))
                {
                    string removed(1, current->data[column]);

                    Action action(
                        DELETE_CHAR, line, column, removed,
                        line, column, line, column);

                    history.record(action);

                    document.deleteCharacterAt(line, column);
                }
                else if (line + 1 < document.getLineCount())
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

            continue;
        }

        switch (key)
        {
        case 1: // Ctrl+A: SAVE AS
        {
            string newFilename = getFilename(true);

            if (!newFilename.empty())
            {
                if (FileManager::saveFile(document, newFilename))
                {
                    filename = newFilename;
                }
            }

            // The editor will redraw on the next iteration.
            break;
        }

        case 0: // Ctrl+Space: SEARCH
        {
            system("cls");

            cout << "\n\n";
            cout << "       =====================================\n";
            cout << "                  SEARCH DOCUMENT\n";
            cout << "       =====================================\n\n";
            cout << "       Enter a word or phrase: ";

            string searchTerm;
            getline(cin, searchTerm);

            system("cls");

            cout << "\n\n";
            cout << "       =====================================\n";
            cout << "                  SEARCH RESULTS\n";
            cout << "       =====================================\n\n";

            if (searchTerm.empty())
            {
                cout << "       Search term cannot be empty.\n";
            }
            else
            {
                cout << "       Search term: " << searchTerm << "\n\n";

                int matches = document.searchText(searchTerm);

                if (matches == 0)
                {
                    cout << "\n       No matches found.\n";
                }
                else
                {
                    cout << "\n       Total matches: " << matches << "\n";
                }
            }

            cout << "\n\n       Press any key to return to the editor...";
            _getch();

            break;
        }

        case 19: // Ctrl+S: SAVE
        {
            if (filename.empty())
            {
                string newFilename = getFilename(true);

                if (!newFilename.empty())
                {
                    if (FileManager::saveFile(document, newFilename))
                    {
                        filename = newFilename;
                    }
                }
            }
            else
            {
                FileManager::saveFile(document, filename);
            }

            break;
        }

        case 15: // Ctrl+O: OPEN
        {
            string newFilename = getFilename(false);

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

        case 26: // Ctrl+Z: UNDO
            history.undo(document);
            break;

        case 25: // Ctrl+Y: REDO
            history.redo(document);
            break;

        case 13: // ENTER: SPLIT LINE
        {
            Action action(
                SPLIT_LINE, line, column, "",
                line, column, line + 1, 0);

            history.record(action);

            document.splitLineAt(line, column);
            document.setCursor(line + 1, 0);

            break;
        }

        case 8: // BACKSPACE
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

                int previousLength =
                    static_cast<int>(previous->data.length());

                Action action(
                    JOIN_LINES, line - 1, previousLength, "",
                    line, column, line - 1, previousLength);

                history.record(action);

                document.joinLineAt(line - 1, previousLength);

                document.setCursor(line - 1, previousLength);
            }

            break;
        }

        default:
        {
            // Insert printable characters.
            if (key >= 32 && key <= 126)
            {
                Action action(
                    INSERT_CHAR, line, column,
                    string(1, static_cast<char>(key)),
                    line, column, line, column + 1);

                history.record(action);

                document.insertCharacterAt(
                    line, column, static_cast<char>(key));

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
    // Set a consistent console title.
    SetConsoleTitleA("C++ Text Editor");

    // Create the folder for text files.
    createDocumentFolder();

    bool applicationRunning = true;

    while (applicationRunning)
    {
        system("cls");

        cout << "\n\n";
        cout << "       +=====================================+\n";
        cout << "       |             TEXT EDITOR             |\n";
        cout << "       +=====================================+\n\n";
        cout << "       1. Create New Document\n\n";
        cout << "       2. Open Existing Document\n\n";
        cout << "       3. Exit\n\n";
        cout << "       +=====================================+\n\n";
        cout << "       Enter your choice: ";

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
            string filename = getFilename(false);

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

    cout << "\n\n";
    cout << "       =====================================\n";
    cout << "       Thank you for using the Text Editor!\n";
    cout << "       =====================================\n\n";

    return 0;
}
