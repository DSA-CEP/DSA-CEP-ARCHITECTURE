#include <iostream>
#include <string>
#include <fstream>
#include <vector>
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

// ================= SMALL TEXT HELPERS =================

// Returns a lowercase copy of the text, used for case-insensitive comparison.
string toLowerCopy(string text)
{
    for (int i = 0; i < text.length(); i++)
    {
        text[i] = tolower((unsigned char)text[i]);
    }

    return text;
}

// One search result, stored as zero-based line and column.
struct SearchMatch
{
    int line;
    int column;
};

// Adds every match found in ONE line to the 'matches' list.
// 'lowerTerm' must already be lowercase (see toLowerCopy).
// We move forward by one character after each match, so overlapping
// matches are counted too (for example "aa" is found twice in "aaa").
// This is the only place where the search loop is written.
void findMatchesInLine(const string &line, const string &lowerTerm,
                       int lineNumber, vector<SearchMatch> &matches)
{
    string lowerLine = toLowerCopy(line);
    size_t position = lowerLine.find(lowerTerm);

    while (position != string::npos)
    {
        SearchMatch match;
        match.line = lineNumber;
        match.column = (int)position;
        matches.push_back(match);

        position = lowerLine.find(lowerTerm, position + 1);
    }
}

// Checks (ignoring upper/lower case) whether 'searchTerm' appears in 'line'
// starting exactly at 'column'. Compares one character at a time.
bool textMatchesAt(const string &line, int column, const string &searchTerm)
{
    if (column < 0 || column + searchTerm.length() > line.length())
    {
        return false;
    }

    for (int i = 0; i < searchTerm.length(); i++)
    {
        char fromLine = tolower((unsigned char)line[column + i]);
        char fromTerm = tolower((unsigned char)searchTerm[i]);

        if (fromLine != fromTerm)
        {
            return false;
        }
    }

    return true;
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

    // Finds every match (case-insensitive) of the term in the whole document.
    // Returns an empty list if the term is empty or nothing matches.
    vector<SearchMatch> findAllMatches(const string &searchTerm)
    {
        vector<SearchMatch> matches;

        if (searchTerm.empty())
        {
            return matches;
        }

        string lowerTerm = toLowerCopy(searchTerm);
        int lineNumber = 0;
        Node *current = head;

        while (current != nullptr)
        {
            findMatchesInLine(current->data, lowerTerm, lineNumber, matches);

            current = current->next;
            lineNumber++;
        }

        return matches;
    }

    // Counts the matches and moves the cursor to the first one.
    // (The editor itself uses findAllMatches() directly.)
    int searchText(const string &searchTerm)
    {
        vector<SearchMatch> matches = findAllMatches(searchTerm);

        if (!matches.empty())
        {
            setCursor(matches[0].line, matches[0].column);
        }

        return matches.size();
    }

    Node *getLineAt(int index)
    {
        if (index < 0 || index >= lineCount)
        {
            return nullptr;
        }

        Node *current;

        // Start from whichever end of the list is closer.
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

    // Breaks one line into two at 'column'. The text after the column
    // moves into a new node that is linked in after the current line.
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

    // Adds the next line to the end of this line and removes the next node.
    // (The column parameter is not needed, it is kept so the calls stay the same.)
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

// ================= MULTI-LINE TEXT (used by merge, delete selection, undo) =================
//
// Multi-line text is stored in an Action as ONE string where the lines
// are separated by '\n'. These helpers convert between that string
// and a list of lines.

// Cuts the text into lines at every '\n'. Always returns at least one line.
vector<string> splitLines(const string &text)
{
    vector<string> lines;
    string currentLine = "";

    for (int i = 0; i < text.length(); i++)
    {
        if (text[i] == '\n')
        {
            lines.push_back(currentLine);
            currentLine = "";
        }
        else
        {
            currentLine += text[i];
        }
    }

    lines.push_back(currentLine);
    return lines;
}

// Opposite of splitLines: joins the lines with '\n' between them.
string linesToText(const vector<string> &lines)
{
    string text = "";

    for (int i = 0; i < lines.size(); i++)
    {
        if (i > 0)
        {
            text += '\n';
        }

        text += lines[i];
    }

    return text;
}

// Inserts multi-line text at line/column.
// Example: inserting "AB\nCD" into "12|34" (cursor at |) gives
//     12AB
//     CD34
// The text before the cursor joins the first inserted line and the
// text after the cursor joins the last inserted line.
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

    // Keep the column inside the line so substr() can never fail.
    int length = first->data.length();

    if (column < 0)
    {
        column = 0;
    }

    if (column > length)
    {
        column = length;
    }

    string before = first->data.substr(0, column); // text left of the cursor
    string after = first->data.substr(column);     // text right of the cursor

    // One line only: just put the text in the middle.
    if (count == 1)
    {
        first->data = before + lines[0] + after;
        return;
    }

    first->data = before + lines[0];

    // Create one new line below the previous one for every extra inserted line.
    for (int i = 1; i < count; i++)
    {
        Node *previous = document.getLineAt(line + i - 1);

        document.splitLineAt(line + i - 1, previous->data.length());
        document.getLineAt(line + i)->data = lines[i];
    }

    // The old text that was after the cursor goes behind the last inserted line.
    document.getLineAt(line + count - 1)->data += after;
}

// Exact reverse of insertMergedText. Also used to delete a selected range.
// 'text' must be exactly the text that is in the document at line/column.
void removeMergedText(Document &document, int line, int column, const string &text)
{
    vector<string> lines = splitLines(text);
    int count = lines.size();
    Node *first = document.getLineAt(line);
    Node *last = document.getLineAt(line + count - 1);

    if (first == nullptr || last == nullptr)
    {
        return;
    }

    // Column where the removed text ends, inside the last affected line.
    int endColumn = lines[count - 1].length();

    if (count == 1)
    {
        endColumn = column + lines[0].length();
    }

    // Keep both columns inside their lines so substr() can never fail.
    if (column > first->data.length())
    {
        column = first->data.length();
    }

    if (endColumn > last->data.length())
    {
        endColumn = last->data.length();
    }

    string before = first->data.substr(0, column); // kept text before the range
    string after = last->data.substr(endColumn);   // kept text after the range

    // Join all affected lines into the first line, then replace its text
    // with just the kept parts.
    for (int i = 1; i < count; i++)
    {
        document.joinLineAt(line, column);
    }

    first->data = before + after;
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

// Returns true if a file or folder with this path already exists.
bool fileExists(const string &path)
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// Shows an error under the filename prompt and waits for a key.
void showFilenameError(string message)
{
    cout << message << "\n";
    cout << "Press any key to try again...";
    _getch();
    cout << "\n";
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
            showFilenameError("ERROR: INVALID FILE EXTENSION. ONLY .txt FILES ARE ALLOWED.");
            continue;
        }

        // Reject paths and Windows-invalid filename characters.
        if (filename.find_first_of("\\/:*?\"<>|") != string::npos)
        {
            showFilenameError("ERROR: INVALID FILENAME. ENTER A NAME, NOT A PATH.");
            continue;
        }

        string fullPath = FOLDER_NAME + filename;

        if (requireUnique && fileExists(fullPath))
        {
            showFilenameError("ERROR: FILE NAME ALREADY USED. PLEASE ENTER A UNIQUE NAME.");
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

    // Is the anchor before the cursor (selecting forward)?
    bool anchorFirst = false;

    if (gSelectionLine < cursorLine)
    {
        anchorFirst = true;
    }
    else if (gSelectionLine == cursorLine && gSelectionColumn <= cursorColumn)
    {
        anchorFirst = true;
    }

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
    if (startLine == endLine && startColumn == endColumn)
    {
        return false;
    }

    return true;
}

// ================= SEARCH RESULTS (shared state) =================

// Every occurrence of the current search term.
static vector<SearchMatch> gSearchMatches;
static int gCurrentSearchMatch = -1;
static string gCurrentSearchTerm;

// Forgets the stored matches. Called after the text changes, because the
// old line/column numbers may no longer be correct.
void clearSearchResults()
{
    gSearchMatches.clear();
    gCurrentSearchMatch = -1;
}

// Text for the status line, for example "MATCH 2 OF 5: hello (CTRL+N/F3 NEXT, F4 LIST)".
string matchInfoText()
{
    return "MATCH " + to_string(gCurrentSearchMatch + 1) + " OF " +
           to_string(gSearchMatches.size()) + ": " + gCurrentSearchTerm +
           " (CTRL+N/F3 NEXT, F4 LIST)";
}

// ================= EDITOR DISPLAY =================

// Writes one row of text to the console at position x, y.
// The text is cut or padded with spaces to exactly 'width' characters,
// so a line never wraps and old text on that row is always overwritten.
void writeConsoleRow(HANDLE console, int x, int y, string text, int width)
{
    if (width <= 0)
        return;

    int length = text.length();

    if (length > width)
        text.resize(width);
    else if (length < width)
        text.append(width - length, ' ');

    COORD position;
    position.X = (SHORT)x;
    position.Y = (SHORT)y;

    DWORD written = 0;
    WriteConsoleOutputCharacterA(console, text.c_str(), (DWORD)width,
                                 position, &written);
}

// Sets the colors of one console row: normal everywhere, highlight colors
// from startX to endX (relative to the row start, endX not included).
void colorConsoleRow(HANDLE console, int x, int y, int width,
                     WORD normal, WORD highlight, int startX, int endX)
{
    DWORD written = 0;

    COORD position;
    position.X = (SHORT)x;
    position.Y = (SHORT)y;
    FillConsoleOutputAttribute(console, normal, (DWORD)width, position, &written);

    if (startX < endX)
    {
        COORD selected;
        selected.X = (SHORT)(x + startX);
        selected.Y = (SHORT)y;
        FillConsoleOutputAttribute(console, highlight, (DWORD)(endX - startX),
                                   selected, &written);
    }
}

// Horizontal scrolling: how many characters of the cursor's line are hidden
// on the left so that the cursor stays visible. 0 means no scrolling.
int getHorizontalOffset(int cursorColumn, int textWidth, int lineLength)
{
    int offset = 0;

    if (cursorColumn >= textWidth)
    {
        offset = cursorColumn - textWidth + 1;
    }

    if (offset > lineLength)
    {
        offset = lineLength;
    }

    return offset;
}

// Draws the 6 rows at the top: title, file name and cursor information.
void drawHeader(HANDLE console, int left, int top, int width,
                const string &filename, Document &document, int cursorLine)
{
    string border(width, '=');
    string heading = "TEXT EDITOR";

    // Put the heading in the middle of the row.
    int headingStart = (width - (int)heading.length()) / 2;
    if (headingStart < 0)
        headingStart = 0;

    string headingRow(width, ' ');
    headingRow.replace(headingStart, heading.length(), heading);

    string shownName = filename;
    if (shownName.empty())
        shownName = "Untitled";

    string infoRow = " Current Line: " + to_string(cursorLine + 1) +
                     " | Column: " + to_string(document.cursorColumn + 1) +
                     " | Total Lines: " + to_string(document.getLineCount()) +
                     " | Characters: " + to_string(document.getCharacterCount());

    writeConsoleRow(console, left, top, border, width);
    writeConsoleRow(console, left, top + 1, headingRow, width);
    writeConsoleRow(console, left, top + 2, border, width);
    writeConsoleRow(console, left, top + 3, " File: " + shownName, width);
    writeConsoleRow(console, left, top + 4, infoRow, width);
    writeConsoleRow(console, left, top + 5, string(width, '-'), width);
}

// Draws the 5 rows at the bottom: shortcut help and the status message.
void drawFooter(HANDLE console, int left, int footerTop, int width,
                const string &status)
{
    string lastRow = " Arrows Move | Enter New Line | Backspace/Delete Remove";
    if (!status.empty())
        lastRow = " " + status;

    writeConsoleRow(console, left, footerTop, string(width, '-'), width);
    writeConsoleRow(console, left, footerTop + 1,
                    " Ctrl+S Save | Ctrl+A Save As | Ctrl+O Merge | Ctrl+F2 Search | F4 List Of Search Results", width);
    writeConsoleRow(console, left, footerTop + 2,
                    " Ctrl+Z Undo | Ctrl+Y Redo | F7 Replace All | F8 Replace One | Esc Menu", width);
    writeConsoleRow(console, left, footerTop + 3,
                    " Shift+Arrows Select | Del/Backspace Delete Selection", width);
    writeConsoleRow(console, left, footerTop + 4, lastRow, width);
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

    const int headerRows = 6;
    const int footerRows = 5;

    int totalLines = document.getLineCount();
    int cursorLine = document.getCursorLineIndex();

    // Every row looks like: " 12 | text". The number column is as wide as
    // the biggest line number, so the "|" signs line up.
    int lineNumberWidth = to_string(totalLines).length();
    int prefixWidth = 1 + lineNumberWidth + 3;

    // Number of screen columns left for the text itself.
    int textWidth = width - prefixWidth;
    if (textWidth < 1)
        textWidth = 1;

    // Vertical scrolling: firstVisibleLine is the document line shown in the
    // first content row. It only moves when the cursor leaves the window.
    int contentRows = height - headerRows - footerRows;
    if (contentRows < 1)
        contentRows = 1;

    static int firstVisibleLine = 0;
    if (cursorLine < firstVisibleLine)
        firstVisibleLine = cursorLine;
    if (cursorLine >= firstVisibleLine + contentRows)
        firstVisibleLine = cursorLine - contentRows + 1;
    if (firstVisibleLine > totalLines - contentRows)
    {
        firstVisibleLine = totalLines - contentRows;
        if (firstVisibleLine < 0)
            firstVisibleLine = 0;
    }

    // Only the cursor's line is scrolled horizontally (when it is very long).
    Node *cursorNode = document.getLineAt(cursorLine);
    int cursorOffset = getHorizontalOffset(document.cursorColumn, textWidth,
                                           cursorNode->data.length());

    drawHeader(console, left, top, width, filename, document, cursorLine);

    // Selected text is shown with inverted colors.
    int selStartLine = 0, selStartColumn = 0, selEndLine = 0, selEndColumn = 0;
    bool hasSelection = getSelectionRange(document, selStartLine, selStartColumn,
                                          selEndLine, selEndColumn);

    // The current search match is highlighted only while the cursor is still
    // on it, so the highlight disappears by itself once the cursor moves or
    // the text under it changes.
    bool hasMatch = false;
    int matchColumn = 0;
    int matchLength = gCurrentSearchTerm.length();

    if (gCurrentSearchMatch >= 0 &&
        gCurrentSearchMatch < gSearchMatches.size() &&
        matchLength > 0)
    {
        SearchMatch match = gSearchMatches[gCurrentSearchMatch];

        if (match.line == cursorLine &&
            match.column == document.cursorColumn &&
            textMatchesAt(cursorNode->data, match.column, gCurrentSearchTerm))
        {
            hasMatch = true;
            matchColumn = match.column;
        }
    }

    // Highlight colors = normal colors with foreground and background swapped.
    WORD normalAttribute = info.wAttributes;
    WORD highlightAttribute = ((normalAttribute & 0x0F) << 4) |
                              ((normalAttribute & 0xF0) >> 4);

    // Draw the document rows.
    Node *current = document.getLineAt(firstVisibleLine);

    for (int row = 0; row < contentRows; row++)
    {
        int screenY = top + headerRows + row;
        int lineIndex = firstVisibleLine + row;

        // Past the last line: draw an empty row (this also erases old text).
        if (current == nullptr)
        {
            writeConsoleRow(console, left, screenY, "", width);
            colorConsoleRow(console, left, screenY, width,
                            normalAttribute, highlightAttribute, 0, 0);
            continue;
        }

        string number = to_string(lineIndex + 1);
        string linePrefix = " " + string(lineNumberWidth - number.length(), ' ') +
                            number + " | ";

        // lineOffset = number of hidden characters on the left of this row.
        string visibleText = current->data;
        int lineOffset = 0;

        if (lineIndex == cursorLine)
        {
            lineOffset = cursorOffset;
            visibleText = visibleText.substr(lineOffset);
        }

        writeConsoleRow(console, left, screenY, linePrefix + visibleText, width);

        // Work out which screen columns of this row get the highlight color.
        // A selected line break is shown as one extra highlighted cell
        // after the text. highlightEnd is not included.
        int highlightStart = 0;
        int highlightEnd = 0;

        if (hasSelection && lineIndex >= selStartLine && lineIndex <= selEndLine)
        {
            int fromColumn = 0;
            if (lineIndex == selStartLine)
                fromColumn = selStartColumn;

            int toColumn = current->data.length() + 1;
            if (lineIndex == selEndLine)
                toColumn = selEndColumn;

            highlightStart = prefixWidth + fromColumn - lineOffset;
            if (highlightStart < prefixWidth)
                highlightStart = prefixWidth;

            highlightEnd = prefixWidth + toColumn - lineOffset;
            if (highlightEnd > width)
                highlightEnd = width;
        }

        // The search match is highlighted only when the selection did not
        // highlight anything on this row.
        if (hasMatch && highlightStart == highlightEnd && lineIndex == cursorLine)
        {
            highlightStart = prefixWidth + matchColumn - lineOffset;
            if (highlightStart < prefixWidth)
                highlightStart = prefixWidth;

            highlightEnd = prefixWidth + matchColumn + matchLength - lineOffset;
            if (highlightEnd > width)
                highlightEnd = width;
        }

        colorConsoleRow(console, left, screenY, width,
                        normalAttribute, highlightAttribute,
                        highlightStart, highlightEnd);

        current = current->next;
    }

    drawFooter(console, left, top + height - footerRows, width, status);

    // Place the blinking cursor and keep it inside the text area.
    int cursorX = left + prefixWidth + document.cursorColumn - cursorOffset;
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

    COORD cursorPosition;
    cursorPosition.X = (SHORT)cursorX;
    cursorPosition.Y = (SHORT)cursorY;
    SetConsoleCursorPosition(console, cursorPosition);
}

// ================= SEARCH =================

// Lists every match of the current search with 1-based line and column
// numbers (the editor header also counts lines and columns from 1).
void showSearchResults()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

    // How many matches fit on one screen page.
    int height = info.srWindow.Bottom - info.srWindow.Top + 1;
    int pageSize = height - 8;
    if (pageSize < 1)
        pageSize = 1;

    int total = gSearchMatches.size();

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

// Shared by Ctrl+F2 and F4: moves the cursor to the current match, shows the
// list of all matches and then sets the status line.
void showFoundMatches(Document &document, string &status)
{
    SearchMatch match = gSearchMatches[gCurrentSearchMatch];
    document.setCursor(match.line, match.column);

    showSearchResults();

    status = "FOUND " + to_string(gSearchMatches.size()) + " MATCHES - " +
             matchInfoText();
}

// Asks for a word, finds all matches (Ctrl+Space, F2 and Ctrl+F2).
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

    gCurrentSearchTerm = searchTerm;
    gSearchMatches = document.findAllMatches(searchTerm);
    gCurrentSearchMatch = -1;

    if (gSearchMatches.empty())
    {
        status = "NO MATCHES FOUND: " + searchTerm;
        return;
    }

    // Start at the first match.
    gCurrentSearchMatch = 0;
    showFoundMatches(document, status);
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

    gSearchMatches = document.findAllMatches(gCurrentSearchTerm);

    if (gSearchMatches.empty())
    {
        gCurrentSearchMatch = -1;
        status = "NO MATCHES FOUND: " + gCurrentSearchTerm;
        return;
    }

    // Keep the current match number if it still exists.
    if (gCurrentSearchMatch < 0 || gCurrentSearchMatch >= gSearchMatches.size())
    {
        gCurrentSearchMatch = 0;
    }

    showFoundMatches(document, status);
}

// Ctrl+N and F3: moves to the next match (after the last one it wraps to the first).
void goToNextSearchMatch(Document &document, string &status)
{
    if (gSearchMatches.empty() || gCurrentSearchMatch < 0)
    {
        status = "SEARCH FIRST USING CTRL+F2";
        return;
    }

    gCurrentSearchMatch++;

    if (gCurrentSearchMatch >= gSearchMatches.size())
    {
        gCurrentSearchMatch = 0;
    }

    SearchMatch match = gSearchMatches[gCurrentSearchMatch];
    document.setCursor(match.line, match.column);

    status = matchInfoText();
}

// ================= REPLACE =================

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

    vector<SearchMatch> matches = document.findAllMatches(searchTerm);

    if (matches.empty())
    {
        status = "NO MATCHES FOUND: " + searchTerm;
        return;
    }

    // Remember the cursor so Esc can put it back.
    int originalLine = document.getCursorLineIndex();
    int originalColumn = document.cursorColumn;

    int total = matches.size();
    int selected = 0;

    // Let the user choose a match. The editor is redrawn each time so the
    // cursor and highlight jump to the selected match.
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

        // Enter confirms the selected match.
        if (key == 13)
        {
            break;
        }

        // Arrow keys come as two codes: 0 or 224, then the real code.
        if (key == 0 || key == 224)
        {
            key = _getch();

            if (key == 72) // Up (wraps from the first match to the last)
            {
                selected--;
                if (selected < 0)
                    selected = total - 1;
            }
            else if (key == 80) // Down (wraps from the last match to the first)
            {
                selected++;
                if (selected >= total)
                    selected = 0;
            }
        }
    }

    SearchMatch match = matches[selected];
    Node *target = document.getLineAt(match.line);

    // Save the whole line before and after, so undo/redo are exact.
    LineChange change;
    change.line = match.line;
    change.before = target->data;
    change.after = target->data;
    change.after.replace(match.column, searchTerm.length(), replacement);

    int afterColumn = match.column + replacement.length();

    // Undo puts the cursor back on the match, redo puts it after the new text.
    Action action(
        REPLACE_TEXT, match.line, match.column, replacement,
        match.line, match.column, match.line, afterColumn);

    action.changes.push_back(change);

    // Nothing to record if the replacement is the same as the old text.
    if (change.before != change.after)
    {
        target->data = change.after;
        history.record(action);
    }

    document.setCursor(match.line, afterColumn);

    clearSearchResults();

    status = "REPLACED MATCH " + to_string(selected + 1) + " OF " +
             to_string(total) + " WITH: " + replacement;
}

// Replaces every occurrence of 'term' in ONE line and returns the new line.
// Matches are replaced from left to right and never overlap, and the
// replaced text is never searched again. 'count' is increased by the
// number of replacements made in this line.
string replaceInLine(const string &line, const string &term,
                     const string &replacement, int &count)
{
    string lowerLine = toLowerCopy(line);
    string lowerTerm = toLowerCopy(term);

    string newLine = "";
    size_t copyFrom = 0; // everything before this position is already copied

    size_t position = lowerLine.find(lowerTerm);
    while (position != string::npos)
    {
        newLine += line.substr(copyFrom, position - copyFrom); // text before the match
        newLine += replacement;                                // replaced text

        copyFrom = position + term.length();
        count++;

        position = lowerLine.find(lowerTerm, copyFrom);
    }

    newLine += line.substr(copyFrom); // text after the last match
    return newLine;
}

// Replace All (F7): replaces every occurrence in one
// undoable action.
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

    // ONE action holds the changes of all lines, so a single Ctrl+Z undoes
    // everything and a single Ctrl+Y redoes everything.
    Action action(
        REPLACE_TEXT, originalLine, originalColumn, replacement,
        originalLine, originalColumn, originalLine, originalColumn);

    int replacementCount = 0;
    int lineNumber = 0;
    Node *current = document.getLineAt(0);

    while (current != nullptr)
    {
        string newText = replaceInLine(current->data, searchTerm,
                                       replacement, replacementCount);

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

        current = current->next;
        lineNumber++;
    }

    if (replacementCount == 0)
    {
        status = "NO MATCHES FOUND: " + searchTerm;
        return;
    }

    if (!action.changes.empty())
    {
        history.record(action);
    }

    // Line lengths may have changed, so setCursor keeps the column valid.
    document.setCursor(originalLine, originalColumn);

    clearSearchResults();

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

    // Copy the lines of the source document into a list.
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
    string text = linesToText(lines);

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

    clearSearchResults();

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

        // The first line starts at the selection start, the last line ends
        // at the selection end. Lines in between are taken completely.
        int from = 0;
        if (i == startLine)
            from = startColumn;

        int to = node->data.length();
        if (i == endLine)
            to = endColumn;

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

    clearSearchResults();

    status = "SELECTION DELETED (CTRL+Z TO UNDO)";
    return true;
}

// ================= SAVE =================

// Ctrl + S: saves to the current file. A new document asks for a name first.
void saveDocument(Document &document, string &filename, string &status)
{
    if (filename.empty())
    {
        string newFilename = getFilename(true);

        if (!newFilename.empty())
            filename = newFilename;
    }

    if (filename.empty())
        return;

    if (FileManager::saveFile(document, filename))
        status = "DOCUMENT SAVED SUCCESSFULLY";
    else
        status = "ERROR: DOCUMENT COULD NOT BE SAVED";
}

// Ctrl + A: Save As must always let the user choose a different name,
// even if the document has already been saved before.
void saveDocumentAs(Document &document, string &filename, string &status)
{
    string newFilename = getFilename(false);

    if (newFilename.empty())
    {
        status = "SAVE AS CANCELLED";
        return;
    }

    // If this is a different existing file, ask before replacing it.
    if (fileExists(newFilename) && newFilename != filename)
    {
        system("cls");
        cout << "The file already exists:\n"
             << newFilename << "\n\n";
        cout << "Replace this file? (Y/N): ";
        int answer = _getch();
        cout << (char)answer << "\n";

        if (answer != 'y' && answer != 'Y')
        {
            status = "SAVE AS CANCELLED - ORIGINAL FILE KEPT";
            return;
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

// ================= TEXT EDITOR =================

// Moves the cursor for the Up, Down, Left, Right, Home and End keys.
// 'line' and 'column' are the cursor position before the move.
void moveCursor(Document &document, int key, int line, int column)
{
    Node *current = document.getLineAt(line);

    switch (key)
    {
    case 72: // Up
        if (line > 0)
        {
            document.setCursor(line - 1, column);
        }
        break;

    case 80: // Down
        if (current->next != nullptr)
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
            // At the start of a line: go to the end of the line above.
            Node *previous = document.getLineAt(line - 1);

            document.setCursor(line - 1, previous->data.length());
        }
        break;

    case 77: // Right
        if (column < current->data.length())
        {
            document.setCursor(line, column + 1);
        }
        else if (current->next != nullptr)
        {
            // At the end of a line: go to the start of the next line.
            document.setCursor(line + 1, 0);
        }
        break;

    case 71: // Home
        document.setCursor(line, 0);
        break;

    case 79: // End
        document.setCursor(line, current->data.length());
        break;
    }
}

// Delete key: removes the character under the cursor, or joins the next line
// to this one when the cursor is at the end of the line.
void deleteKeyPressed(Document &document, UndoRedo &history, int line, int column)
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
}

// Backspace key: removes the character before the cursor, or joins this
// line to the end of the line above when the cursor is at the line start.
void backspacePressed(Document &document, UndoRedo &history, int line, int column)
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

        document.setCursor(line - 1, previousLength);
    }
}

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

        // Arrow keys, Delete and F-keys send two codes: 0 or 224 first,
        // then the real key code.
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
                // The selection starts where the cursor was before the move.
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

            // Search: Ctrl+Space (can arrive as NUL followed by ASCII space,
            // code 32), F2 (code 60) and Ctrl+F2 (code 0x5F on MSVC).
            if (key == 32 || key == 60 || key == 0x5F)
            {
                searchDocument(document, status);
                continue;
            }

            // F3 (scan code 61) moves to the next match.
            if (key == 61)
            {
                goToNextSearchMatch(document, status);
                continue;
            }

            // F4 (scan code 62) lists every match of the last search.
            if (key == 62)
            {
                listSearchMatches(document, status);
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

            if (key == 83) // Delete
            {
                deleteKeyPressed(document, history, line, column);
            }
            else
            {
                moveCursor(document, key, line, column);
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
                saveDocumentAs(document, filename, status);
                break;

            case 14: // Ctrl + N - Next search match (reliable fallback for F3)
                goToNextSearchMatch(document, status);
                break;

            case 19: // Ctrl + S - Save
                saveDocument(document, filename, status);
                break;

            case 15: // Ctrl + O - Merge another document at the cursor
                mergeDocument(document, history, status);
                break;

            case 26: // Ctrl + Z
                history.undo(document);
                break;

            case 25: // Ctrl + Y
                history.redo(document);
                break;

            case 13: // Enter - split the line at the cursor
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
                backspacePressed(document, history, line, column);
                break;

            default:
                // Normal printable characters.
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