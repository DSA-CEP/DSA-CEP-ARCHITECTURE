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
const int REPLACE_TEXT = 5;
const int MERGE_TEXT = 6;
const int DELETE_RANGE = 7;

// Displays a message and waits for a key.
void showMessage(string message)
{
    system("cls");
    cout << message << endl;
    cout << "\nPress any key to continue...";
    _getch();
}

// Stores the full text of one line before and after a replacement.
// Used by REPLACE_TEXT so one Action can undo/redo many changed lines.
struct LineChange
{
    int line;
    string before;
    string after;
};

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

    // Only used by REPLACE_TEXT (one entry per changed line).
    vector<LineChange> changes;

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
                  [](unsigned char ch)
                  { return static_cast<char>(tolower(ch)); });

        int matchCount = 0;
        int lineNumber = 0;
        int firstMatchLine = -1;
        int firstMatchColumn = -1;
        Node *current = head;

        while (current != nullptr)
        {
            string foldedLine = current->data;
            transform(foldedLine.begin(), foldedLine.end(), foldedLine.begin(),
                      [](unsigned char ch)
                      { return static_cast<char>(tolower(ch)); });

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

// Splits text at every '\n'. Merged content is stored in an Action as
// lines joined by '\n', so undo and redo can rebuild it.
vector<string> splitLines(const string &text)
{
    vector<string> lines;
    size_t start = 0;
    size_t end = text.find('\n');

    while (end != string::npos)
    {
        lines.push_back(text.substr(start, end - start));
        start = end + 1;
        end = text.find('\n', start);
    }

    lines.push_back(text.substr(start));
    return lines;
}

// Inserts multi-line text at line/column. The text before the cursor joins
// the first inserted line and the text after the cursor joins the last one.
// Also used to undo a selection deletion.
void insertMergedText(Document &document, int line, int column, const string &text)
{
    vector<string> lines = splitLines(text);
    int count = lines.size();
    Node *first = document.getLineAt(line);

    if (first == nullptr)
    {
        return;
    }

    if (count == 1)
    {
        first->data.insert(column, lines[0]);
        return;
    }

    // Break the line at the cursor, then fill in the new lines.
    document.splitLineAt(line, column);
    first->data += lines[0];

    for (int i = 1; i < count - 1; i++)
    {
        Node *previous = document.getLineAt(line + i - 1);

        document.splitLineAt(line + i - 1, previous->data.length());
        document.getLineAt(line + i)->data = lines[i];
    }

    document.getLineAt(line + count - 1)->data.insert(0, lines[count - 1]);
}

// Exact reverse of insertMergedText. Also used to delete a selected range.
void removeMergedText(Document &document, int line, int column, const string &text)
{
    vector<string> lines = splitLines(text);
    int count = lines.size();
    Node *first = document.getLineAt(line);

    if (first == nullptr)
    {
        return;
    }

    if (count == 1)
    {
        first->data.erase(column, lines[0].length());
        return;
    }

    Node *last = document.getLineAt(line + count - 1);

    // Keep only the original text before and after the merged content.
    first->data.erase(column);
    last->data.erase(0, lines[count - 1].length());

    for (int i = 1; i < count - 1; i++)
    {
        document.getLineAt(line + i)->data = "";
    }

    for (int i = 1; i < count; i++)
    {
        document.joinLineAt(line, column);
    }
}

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

        case REPLACE_TEXT:
            // Put every changed line back to its original text.
            for (int i = 0; i < action.changes.size(); i++)
            {
                Node *target = document.getLineAt(action.changes[i].line);

                if (target != nullptr)
                {
                    target->data = action.changes[i].before;
                }
            }
            break;

        case MERGE_TEXT:
            // Remove the whole merged block in one step.
            removeMergedText(
                document, action.line, action.column, action.text);
            break;

        case DELETE_RANGE:
            // Put the deleted text back exactly where it was.
            insertMergedText(
                document, action.line, action.column, action.text);
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

        case REPLACE_TEXT:
            // Re-apply the replaced text to every changed line.
            for (int i = 0; i < action.changes.size(); i++)
            {
                Node *target = document.getLineAt(action.changes[i].line);

                if (target != nullptr)
                {
                    target->data = action.changes[i].after;
                }
            }
            break;

        case MERGE_TEXT:
            // Insert the whole merged block again in one step.
            insertMergedText(
                document, action.line, action.column, action.text);
            break;

        case DELETE_RANGE:
            // Delete the same range again in one step.
            removeMergedText(
                document, action.line, action.column, action.text);
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

// ================= SELECTION =================

// The selection runs from the anchor (where Shift was first pressed)
// to the current cursor position.
static bool gSelectionActive = false;
static int gSelectionLine = 0;
static int gSelectionColumn = 0;

void clearSelection()
{
    gSelectionActive = false;
}

// Gets the selection as start/end positions (start is always before end).
// Returns false when nothing is selected or the selection is empty.
bool getSelectionRange(Document &document, int &startLine, int &startColumn,
                       int &endLine, int &endColumn)
{
    if (!gSelectionActive)
    {
        return false;
    }

    int cursorLine = document.getCursorLineIndex();
    int cursorColumn = document.cursorColumn;

    bool anchorFirst = gSelectionLine < cursorLine ||
                       (gSelectionLine == cursorLine &&
                        gSelectionColumn <= cursorColumn);

    if (anchorFirst)
    {
        startLine = gSelectionLine;
        startColumn = gSelectionColumn;
        endLine = cursorLine;
        endColumn = cursorColumn;
    }
    else
    {
        startLine = cursorLine;
        startColumn = cursorColumn;
        endLine = gSelectionLine;
        endColumn = gSelectionColumn;
    }

    // Anchor and cursor on the same spot means an empty selection.
    return !(startLine == endLine && startColumn == endColumn);
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

// ================= EDITOR DISPLAY =================

void writeConsoleRow(HANDLE console, short x, short y, const string &value, int width)
{
    if (width <= 0)
        return;

    string row = value;
    if ((int)row.length() > width)
        row.resize(width); // Never allow a line to wrap.
    if ((int)row.length() < width)
        row.append(width - row.length(), ' ');

    COORD position = {x, y};
    DWORD written = 0;
    WriteConsoleOutputCharacterA(console, row.c_str(), (DWORD)width,
                                 position, &written);
}

// Sets the colors of one console row: normal everywhere, highlight colors
// from startX to endX (relative to the row start, endX not included).
void colorConsoleRow(HANDLE console, short x, short y, int width,
                     WORD normal, WORD highlight, int startX, int endX)
{
    DWORD written = 0;
    COORD position = {x, y};
    FillConsoleOutputAttribute(console, normal, (DWORD)width, position, &written);

    if (startX < endX)
    {
        COORD selected = {(SHORT)(x + startX), y};
        FillConsoleOutputAttribute(console, highlight, (DWORD)(endX - startX),
                                   selected, &written);
    }
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

    if (width < 20 || height < 11)
        return;

    int totalLines = document.getLineCount();
    int lineNumberWidth = 1;
    for (int n = totalLines; n >= 10; n /= 10)
        lineNumberWidth++;

    const int headerRows = 6;
    const int footerRows = 5;
    int contentRows = height - headerRows - footerRows;
    if (contentRows < 1)
        contentRows = 1;

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

    // Selected text is shown with inverted colors.
    int selStartLine = 0, selStartColumn = 0, selEndLine = 0, selEndColumn = 0;
    bool hasSelection = getSelectionRange(document, selStartLine, selStartColumn,
                                          selEndLine, selEndColumn);

    // The current search match is highlighted while the cursor is still on it,
    // so the highlight disappears by itself once the cursor moves or the text
    // under it changes.
    bool hasMatch = false;
    int matchColumn = 0;
    int matchLength = static_cast<int>(gCurrentSearchTerm.length());

    if (gCurrentSearchMatch >= 0 &&
        gCurrentSearchMatch < static_cast<int>(gSearchMatches.size()) &&
        matchLength > 0)
    {
        const SearchMatch &match = gSearchMatches[gCurrentSearchMatch];
        Node *matchLine = document.getLineAt(match.line);

        if (matchLine != nullptr && match.line == cursorLine &&
            match.column == document.cursorColumn &&
            match.column + matchLength <= static_cast<int>(matchLine->data.length()) &&
            equal(gCurrentSearchTerm.begin(), gCurrentSearchTerm.end(),
                  matchLine->data.begin() + match.column,
                  [](char a, char b)
                  { return tolower(static_cast<unsigned char>(a)) ==
                           tolower(static_cast<unsigned char>(b)); }))
        {
            hasMatch = true;
            matchColumn = match.column;
        }
    }

    WORD normalAttribute = info.wAttributes;
    WORD highlightAttribute = ((normalAttribute & 0x0F) << 4) |
                              ((normalAttribute & 0xF0) >> 4);

    for (int row = 0; row < contentRows; row++)
    {
        int lineIndex = firstVisibleLine + row;

        // Always rewrite each document row. Empty rows are cleared by
        // writeConsoleRow's padding, without clearing the whole terminal.
        if (lineIndex >= totalLines)
        {
            writeConsoleRow(console, (short)left,
                            (short)(top + headerRows + row), "", width);
            colorConsoleRow(console, (short)left, (short)(top + headerRows + row),
                            width, normalAttribute, highlightAttribute, 0, 0);
            continue;
        }

        Node *current = document.getLineAt(lineIndex);
        string number = to_string(lineIndex + 1);
        string linePrefix = " " + string(lineNumberWidth - number.length(), ' ') +
                            number + " | ";
        string visibleText = current->data;
        int lineOffset = 0;

        // If the cursor is far across a long line, horizontally scroll that
        // line so the actual cursor/search match remains visible.
        if (lineIndex == cursorLine)
        {
            int textWidth = max(1, width - prefixWidth);
            int horizontalOffset = max(0, document.cursorColumn - textWidth + 1);
            if (horizontalOffset > static_cast<int>(visibleText.length()))
                horizontalOffset = static_cast<int>(visibleText.length());
            visibleText = visibleText.substr(horizontalOffset);
            lineOffset = horizontalOffset;
        }

        writeConsoleRow(console, (short)left,
                        (short)(top + headerRows + row), linePrefix + visibleText, width);

        // Work out which screen columns of this row are selected. A selected
        // line break is shown as one extra highlighted cell after the text.
        int highlightStart = 0;
        int highlightEnd = 0;

        if (hasSelection && lineIndex >= selStartLine && lineIndex <= selEndLine)
        {
            int fromColumn = (lineIndex == selStartLine) ? selStartColumn : 0;
            int toColumn = (lineIndex == selEndLine) ? selEndColumn
                                                     : static_cast<int>(current->data.length()) + 1;

            highlightStart = max(prefixWidth, prefixWidth + fromColumn - lineOffset);
            highlightEnd = min(width, prefixWidth + toColumn - lineOffset);
        }

        if (hasMatch && highlightStart == highlightEnd && lineIndex == cursorLine)
        {
            highlightStart = max(prefixWidth, prefixWidth + matchColumn - lineOffset);
            highlightEnd = min(width, prefixWidth + matchColumn + matchLength - lineOffset);
        }

        colorConsoleRow(console, (short)left, (short)(top + headerRows + row),
                        width, normalAttribute, highlightAttribute,
                        highlightStart, highlightEnd);
    }

    int footerTop = top + height - footerRows;
    writeConsoleRow(console, (short)left, (short)footerTop, string(width, '-'), width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 1),
                    " Ctrl+S Save | Ctrl+A Save As | Ctrl+O Merge | Ctrl+F2 Search | F4 List Of Search Results", width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 2),
                    " Ctrl+Z Undo | Ctrl+Y Redo | F7 Replace All | F8 Replace One | Esc Menu", width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 3),
                    " Shift+Arrows Select | Del/Backspace Delete Selection", width);
    writeConsoleRow(console, (short)left, (short)(footerTop + 4),
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
    if (cursorX > maxX)
        cursorX = maxX;
    if (cursorX < left)
        cursorX = left;
    if (cursorY > maxY)
        cursorY = maxY;
    if (cursorY < top + headerRows)
        cursorY = top + headerRows;

    COORD cursorPosition = {(SHORT)cursorX, (SHORT)cursorY};
    SetConsoleCursorPosition(console, cursorPosition);
}

// Lists every match of the current search with 1-based line and column
// numbers (the editor header also counts lines and columns from 1).
void showSearchResults()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

    int height = info.srWindow.Bottom - info.srWindow.Top + 1;
    int pageSize = max(1, height - 8);
    int total = static_cast<int>(gSearchMatches.size());

    system("cls");
    cout << "========== SEARCH RESULTS ==========" << endl;
    cout << "Search term: " << gCurrentSearchTerm << endl;
    cout << "Total matches: " << total << endl;
    cout << "------------------------------------" << endl;

    for (int i = 0; i < total; i++)
    {
        cout << "Match " << (i + 1) << ": Line " << (gSearchMatches[i].line + 1)
             << ", Column " << (gSearchMatches[i].column + 1) << endl;

        // Pause when the screen is full so nothing scrolls out of view.
        if ((i + 1) % pageSize == 0 && i + 1 < total)
        {
            cout << "-- " << (i + 1) << " of " << total
                 << " shown. Press any key for more, Esc to stop --";

            int key = _getch();

            if (key == 0 || key == 224)
                _getch(); // Skip the second code of an arrow/function key.

            cout << endl;

            if (key == 27)
                break;
        }
    }

    cout << "\nPress any key to return to the editor...";

    int key = _getch();

    if (key == 0 || key == 224)
        _getch();
}

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
              [](unsigned char ch)
              { return static_cast<char>(tolower(ch)); });

    int lineNumber = 0;
    Node *current = document.getLineAt(0);

    while (current != nullptr)
    {
        string foldedLine = current->data;
        transform(foldedLine.begin(), foldedLine.end(), foldedLine.begin(),
                  [](unsigned char ch)
                  { return static_cast<char>(tolower(ch)); });

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

    // Show every match location, then return to the editor at match 1.
    showSearchResults();

    status = "FOUND " + to_string(gSearchMatches.size()) + " MATCHES - MATCH 1 OF " +
             to_string(gSearchMatches.size()) + ": " + searchTerm +
             " (CTRL+N/F3 NEXT, F4 LIST)";
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
             " (CTRL+N/F3 NEXT, F4 LIST)";
}

// Moves back to the previous match (wraps from the first to the last).
// Still used by Shift+F3.
void goToPreviousSearchMatch(Document &document, string &status)
{
    if (gSearchMatches.empty() || gCurrentSearchMatch < 0)
    {
        status = "SEARCH FIRST USING CTRL+F2";
        return;
    }

    int total = static_cast<int>(gSearchMatches.size());

    gCurrentSearchMatch = (gCurrentSearchMatch - 1 + total) % total;

    const SearchMatch &match = gSearchMatches[gCurrentSearchMatch];
    document.setCursor(match.line, match.column);

    status = "MATCH " + to_string(gCurrentSearchMatch + 1) + " OF " +
             to_string(gSearchMatches.size()) + ": " + gCurrentSearchTerm +
             " (CTRL+N/F3 NEXT, F4 LIST)";
}

// ================= REPLACE =================

// Returns a lowercase copy of the text for case-insensitive comparison.
string toLowerCopy(string text)
{
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char ch)
              { return static_cast<char>(tolower(ch)); });

    return text;
}

// Asks for the search term and the replacement term.
// Returns false if the search term is empty (operation cancelled).
// An empty replacement is allowed and simply deletes the matched text.
bool getReplaceTerms(string title, string &searchTerm, string &replacement)
{
    system("cls");
    cout << "========== " << title << " ==========" << endl;
    cout << "Enter a word or phrase to find: ";
    getline(cin, searchTerm);

    if (searchTerm.empty())
    {
        return false;
    }

    cout << "Enter the replacement (leave empty to delete): ";
    getline(cin, replacement);

    return true;
}

// Finds every occurrence (case-insensitive, overlapping, same as search).
vector<SearchMatch> findAllMatches(Document &document, const string &searchTerm)
{
    vector<SearchMatch> matches;
    string foldedTerm = toLowerCopy(searchTerm);

    int lineNumber = 0;
    Node *current = document.getLineAt(0);

    while (current != nullptr)
    {
        string foldedLine = toLowerCopy(current->data);

        size_t position = foldedLine.find(foldedTerm);
        while (position != string::npos)
        {
            SearchMatch match;
            match.line = lineNumber;
            match.column = static_cast<int>(position);
            matches.push_back(match);

            position = foldedLine.find(foldedTerm, position + 1);
        }

        current = current->next;
        ++lineNumber;
    }

    return matches;
}

// F4: shows the match list of the last search again. The matches are found
// again first, so edits made since the search are included.
void listSearchMatches(Document &document, string &status)
{
    if (gCurrentSearchTerm.empty())
    {
        status = "SEARCH FIRST USING CTRL+F2";
        return;
    }

    gSearchMatches = findAllMatches(document, gCurrentSearchTerm);

    if (gSearchMatches.empty())
    {
        gCurrentSearchMatch = -1;
        status = "NO MATCHES FOUND: " + gCurrentSearchTerm;
        return;
    }

    if (gCurrentSearchMatch < 0 ||
        gCurrentSearchMatch >= static_cast<int>(gSearchMatches.size()))
    {
        gCurrentSearchMatch = 0;
    }

    document.setCursor(gSearchMatches[gCurrentSearchMatch].line,
                       gSearchMatches[gCurrentSearchMatch].column);

    showSearchResults();

    status = "FOUND " + to_string(gSearchMatches.size()) + " MATCHES - MATCH " +
             to_string(gCurrentSearchMatch + 1) + " OF " +
             to_string(gSearchMatches.size()) + ": " + gCurrentSearchTerm +
             " (CTRL+N/F3 NEXT, F4 LIST)";
}

// Replace One (F8): the user picks one occurrence with Up/Down,
// Enter replaces it, Esc cancels without changing anything.
void replaceOneOccurrence(Document &document, UndoRedo &history,
                          string filename, string &status)
{
    string searchTerm;
    string replacement;

    if (!getReplaceTerms("REPLACE ONE OCCURRENCE", searchTerm, replacement))
    {
        status = "REPLACE CANCELLED - EMPTY SEARCH TERM";
        return;
    }

    vector<SearchMatch> matches = findAllMatches(document, searchTerm);

    if (matches.empty())
    {
        status = "NO MATCHES FOUND: " + searchTerm;
        return;
    }

    // Remember the cursor so Esc can put it back.
    int originalLine = document.getCursorLineIndex();
    int originalColumn = document.cursorColumn;

    int total = static_cast<int>(matches.size());
    int selected = 0;

    while (true)
    {
        document.setCursor(matches[selected].line, matches[selected].column);

        displayEditor(document, filename,
                      "FOUND " + to_string(total) + " | MATCH " +
                          to_string(selected + 1) + " OF " + to_string(total) +
                          " | UP/DOWN: MOVE | ENTER: REPLACE | ESC: CANCEL");

        int key = _getch();

        // ESC cancels without touching the document.
        if (key == 27)
        {
            document.setCursor(originalLine, originalColumn);
            status = "REPLACE CANCELLED - DOCUMENT NOT CHANGED";
            return;
        }

        if (key == 0 || key == 224)
        {
            key = _getch();

            if (key == 72) // Up
            {
                selected = (selected - 1 + total) % total;
            }
            else if (key == 80) // Down
            {
                selected = (selected + 1) % total;
            }
        }
        else if (key == 13) // Enter
        {
            break;
        }
    }

    SearchMatch match = matches[selected];
    Node *target = document.getLineAt(match.line);

    LineChange change;
    change.line = match.line;
    change.before = target->data;
    change.after = target->data;
    change.after.replace(match.column, searchTerm.length(), replacement);

    int afterColumn = match.column + static_cast<int>(replacement.length());

    // Undo puts the cursor back on the match, redo puts it after the new text.
    Action action(
        REPLACE_TEXT, match.line, match.column, replacement,
        match.line, match.column, match.line, afterColumn);

    action.changes.push_back(change);

    if (change.before != change.after)
    {
        target->data = change.after;
        history.record(action);
    }

    document.setCursor(match.line, afterColumn);

    // Old search results may no longer point to the right columns.
    gSearchMatches.clear();
    gCurrentSearchMatch = -1;

    status = "REPLACED MATCH " + to_string(selected + 1) + " OF " +
             to_string(total) + " WITH: " + replacement;
}

// Replace All (F7): replaces every occurrence in one
// undoable action. Matches are replaced left to right without overlapping,
// and replaced text is never searched again.
void replaceAllOccurrences(Document &document, UndoRedo &history,
                           string &status)
{
    string searchTerm;
    string replacement;

    if (!getReplaceTerms("REPLACE ALL OCCURRENCES", searchTerm, replacement))
    {
        status = "REPLACE ALL CANCELLED - EMPTY SEARCH TERM";
        return;
    }

    int originalLine = document.getCursorLineIndex();
    int originalColumn = document.cursorColumn;

    string foldedTerm = toLowerCopy(searchTerm);

    Action action(
        REPLACE_TEXT, originalLine, originalColumn, replacement,
        originalLine, originalColumn, originalLine, originalColumn);

    int replacementCount = 0;
    int lineNumber = 0;
    Node *current = document.getLineAt(0);

    while (current != nullptr)
    {
        string foldedLine = toLowerCopy(current->data);

        string newText = "";
        size_t copyFrom = 0;
        int lineReplacements = 0;

        size_t position = foldedLine.find(foldedTerm);
        while (position != string::npos)
        {
            newText += current->data.substr(copyFrom, position - copyFrom);
            newText += replacement;

            copyFrom = position + searchTerm.length();
            ++lineReplacements;

            position = foldedLine.find(foldedTerm, copyFrom);
        }

        if (lineReplacements > 0)
        {
            newText += current->data.substr(copyFrom);
            replacementCount += lineReplacements;

            // Only lines whose text really changes go into the undo action.
            if (newText != current->data)
            {
                LineChange change;
                change.line = lineNumber;
                change.before = current->data;
                change.after = newText;
                action.changes.push_back(change);

                current->data = newText;
            }
        }

        current = current->next;
        ++lineNumber;
    }

    if (replacementCount == 0)
    {
        status = "NO MATCHES FOUND: " + searchTerm;
        return;
    }

    // One action for the whole operation = one Ctrl+Z / one Ctrl+Y.
    if (!action.changes.empty())
    {
        history.record(action);
    }

    // Line lengths may have changed, so setCursor keeps the column valid.
    document.setCursor(originalLine, originalColumn);

    // Old search results may no longer point to the right columns.
    gSearchMatches.clear();
    gCurrentSearchMatch = -1;

    status = "REPLACED " + to_string(replacementCount) + " OCCURRENCE(S) OF: " +
             searchTerm + " (CTRL+Z TO UNDO)";
}

// ================= MERGE =================

// Merge Document (Ctrl+O): inserts another .txt file at the cursor.
// The source file is only read, never changed, and the current document
// keeps its filename. The merge is stored as ONE undo action.
void mergeDocument(Document &document, UndoRedo &history, string &status)
{
    string sourceName = getFilename();

    if (sourceName.empty())
    {
        status = "MERGE CANCELLED";
        return;
    }

    // Read the source into a temporary document so the current one is kept.
    Document source;

    if (!FileManager::openFile(source, sourceName))
    {
        status = "MERGE FAILED - NOTHING WAS CHANGED";
        return;
    }

    vector<string> lines;
    Node *current = source.getLineAt(0);

    while (current != nullptr)
    {
        lines.push_back(current->data);
        current = current->next;
    }

    // An empty file is loaded as one empty line, so there is nothing to add.
    if (lines.size() == 1 && lines[0].empty())
    {
        status = "NOTHING TO MERGE - SOURCE FILE IS EMPTY";
        return;
    }

    int count = lines.size();
    string text = "";

    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            text += '\n';
        }

        text += lines[i];
    }

    int line = document.getCursorLineIndex();
    int column = document.cursorColumn;

    // The cursor ends right after the last inserted character.
    int afterLine = line + count - 1;
    int afterColumn = lines[count - 1].length();

    if (count == 1)
    {
        afterColumn = column + lines[0].length();
    }

    Action action(
        MERGE_TEXT, line, column, text,
        line, column, afterLine, afterColumn);

    insertMergedText(document, line, column, text);
    history.record(action);

    document.setCursor(afterLine, afterColumn);

    // Old search results may no longer point to the right columns.
    gSearchMatches.clear();
    gCurrentSearchMatch = -1;

    status = "MERGED " + to_string(count) + " LINE(S) FROM " + sourceName +
             " (CTRL+Z TO UNDO)";
}

// ================= DELETE SELECTION =================

// Returns the selected text, with lines joined by '\n' (same format that
// insertMergedText and removeMergedText use).
string getSelectedText(Document &document, int startLine, int startColumn,
                       int endLine, int endColumn)
{
    string text = "";

    for (int i = startLine; i <= endLine; i++)
    {
        Node *node = document.getLineAt(i);

        int from = (i == startLine) ? startColumn : 0;
        int to = (i == endLine) ? endColumn : static_cast<int>(node->data.length());

        text += node->data.substr(from, to - from);

        if (i < endLine)
        {
            text += '\n';
        }
    }

    return text;
}

// Deletes the selected text (one word, a range, or several lines) as ONE
// undoable action. Returns false if nothing is selected, so the caller
// can fall back to the normal single-character delete.
bool deleteSelection(Document &document, UndoRedo &history, string &status)
{
    int startLine, startColumn, endLine, endColumn;

    if (!getSelectionRange(document, startLine, startColumn, endLine, endColumn))
    {
        return false;
    }

    string text = getSelectedText(document, startLine, startColumn,
                                  endLine, endColumn);

    // Undo and redo both put the cursor at the start of the deleted range.
    Action action(
        DELETE_RANGE, startLine, startColumn, text,
        startLine, startColumn, startLine, startColumn);

    removeMergedText(document, startLine, startColumn, text);
    history.record(action);

    document.setCursor(startLine, startColumn);
    clearSelection();

    // Old search results may no longer point to the right columns.
    gSearchMatches.clear();
    gCurrentSearchMatch = -1;

    status = "SELECTION DELETED (CTRL+Z TO UNDO)";
    return true;
}

// ================= TEXT EDITOR =================

void runEditor(Document &document, UndoRedo &history,
               string &filename)
{
    bool running = true;
    string status = "";

    clearSelection();

    while (running)
    {
        // While text is selected, the status row shows the selection shortcuts.
        int selStartLine, selStartColumn, selEndLine, selEndColumn;

        if (getSelectionRange(document, selStartLine, selStartColumn,
                              selEndLine, selEndColumn))
        {
            displayEditor(document, filename,
                          "SELECTED | DEL/BACKSPACE: DELETE | SHIFT+ARROWS: CHANGE | OTHER KEY: CANCEL");
        }
        else
        {
            displayEditor(document, filename, status);
        }

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

            // Shift + a movement key starts or extends the selection.
            // _getch() cannot report Shift, so Windows is asked for its state.
            bool isMoveKey = (key == 72 || key == 80 || key == 75 ||
                              key == 77 || key == 71 || key == 79);
            bool shiftHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

            if (isMoveKey && shiftHeld)
            {
                if (!gSelectionActive)
                {
                    gSelectionActive = true;
                    gSelectionLine = line;
                    gSelectionColumn = column;
                }
            }
            else if (key == 83 && deleteSelection(document, history, status))
            {
                continue; // Delete removed the selected text.
            }
            else
            {
                clearSelection(); // Any other key cancels the selection.
            }

            // Ctrl+Space can be delivered as NUL followed by ASCII space.
            if (key == 32)
            {
                searchDocument(document, status);
                continue;
            }

            // F3 moves to the next occurrence of the active search.
            // F3 scan code is 61 on the standard Windows console.
            if (key == 61)
            {
                goToNextSearchMatch(document, status);
                continue;
            }

            // Shift+F3 (scan code 86) moves to the previous match.
            if (key == 86)
            {
                goToPreviousSearchMatch(document, status);
                continue;
            }

            // F4 (scan code 62) lists every match of the last search.
            if (key == 62)
            {
                listSearchMatches(document, status);
                continue;
            }

            // F2 is also supported as an easy alternative for starting search.
            if (key == 60)
            {
                searchDocument(document, status);
                continue;
            }

            // F7 (scan code 65) replaces every occurrence.
            if (key == 65)
            {
                replaceAllOccurrences(document, history, status);
                continue;
            }

            // F8 (scan code 66) replaces one selected occurrence.
            if (key == 66)
            {
                replaceOneOccurrence(document, history, filename, status);
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
            // Backspace deletes the selected text; any other key cancels it.
            if (key == 8 && deleteSelection(document, history, status))
            {
                continue;
            }

            clearSelection();

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
                        cout << "The file already exists:\n"
                             << newFilename << "\n\n";
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

            case 14: // Ctrl + N - Next search match (reliable fallback for F3)
            {
                goToNextSearchMatch(document, status);
                break;
            }

            case 19: // Ctrl + S
            {
                if (filename.empty())
                {
                    string newFilename = getFilename(true);
                    if (!newFilename.empty())
                        filename = newFilename;
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

            case 15: // Ctrl + O - Merge another document at the cursor
            {
                mergeDocument(document, history, status);
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