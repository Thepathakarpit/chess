#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>     // Linux equivalent for sleep
#include <termios.h>    // Linux terminal handling
#include <sys/select.h> // For non-blocking input
#include <limits.h>     // For INT_MIN

#define BOARD_SIZE 8

// Piece representations
#define EMPTY ' '
#define WHITE_PAWN 'P'
#define WHITE_ROOK 'R'
#define WHITE_KNIGHT 'N'
#define WHITE_BISHOP 'B'
#define WHITE_QUEEN 'Q'
#define WHITE_KING 'K'
#define BLACK_PAWN 'p'
#define BLACK_ROOK 'r'
#define BLACK_KNIGHT 'n'
#define BLACK_BISHOP 'b'
#define BLACK_QUEEN 'q'
#define BLACK_KING 'k'

#define EXIT_KEY 'q'

typedef struct {
    bool whiteCastleKingside;
    bool whiteCastleQueenside;
    bool blackCastleKingside;
    bool blackCastleQueenside;
    int enPassantTargetRow;
    int enPassantTargetCol;
    int halfmoveClock;
    int fullmoveNumber;
    char board[BOARD_SIZE][BOARD_SIZE];
    bool isWhiteTurn;
    bool canWhiteCastleKingside;
    bool canWhiteCastleQueenside;
    bool canBlackCastleKingside;
    bool canBlackCastleQueenside;
} GameState;

// Global variables for cursor handling
int cursorRow = 0;
int cursorCol = 0;
bool pieceSelected = false;
int selectedRow = -1;
int selectedCol = -1;
int aiDepth = 3; // Default AI depth

// Terminal handling for Linux
struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

char getch() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return 0;
}

void clearScreen() {
    printf("\033[2J\033[H"); // ANSI escape sequence to clear screen
}

void sleepMs(int milliseconds) {
    usleep(milliseconds * 1000); // Convert to microseconds
}

// Pin detection structure
typedef struct {
    bool isPinned;
    int pinDirection[2];  // dr, dc (direction of pin)
    int pinningPieceRow;
    int pinningPieceCol;
} PinInfo;

// Move list structure for efficient move generation
typedef struct {
    int moves[256][4];  // [fromRow, fromCol, toRow, toCol]
    int count;
    int scores[256];    // Move ordering scores
} MoveList;

// Simple position cache for evaluation
typedef struct {
    char board[BOARD_SIZE][BOARD_SIZE];
    bool isWhiteTurn;
    int evaluation;
    bool isValid;
} PositionCache;

// Global cache (simple implementation)
PositionCache evalCache[64];
int cacheIndex = 0;

// Function prototypes
void initializeGameState(GameState* state);
void printBoard(char board[BOARD_SIZE][BOARD_SIZE]);
bool isValidMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
void makeMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isGameOver(GameState* state);
PinInfo getPinInfo(GameState* state, int row, int col);
bool isMoveAlongPinRay(int fromRow, int fromCol, int toRow, int toCol, PinInfo pin);
void generateAllMoves(GameState* state, MoveList* moveList);
int scoreMoveForOrdering(GameState* state, int move[4]);
int getCachedEvaluation(GameState* state);
void cacheEvaluation(GameState* state, int evaluation);
bool isScholarsMateThreát(GameState* state);
bool isMateThreateningMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
int evaluateScholarsMateDefense(GameState* state);
bool isValidPawnMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isValidRookMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isValidKnightMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isValidBishopMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isValidQueenMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isValidKingMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isInCheck(GameState* state, bool isWhiteKing);
bool isCheckmate(GameState* state);
bool isKingCapturable(GameState* state, bool isWhiteKing);
void promotePawn(GameState* state, int row, int col);
bool doesMovePutKingInCheck(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isSquareAttacked(GameState* state, int row, int col, bool byWhite);
void getAIMove(GameState* state, int *fromRow, int *fromCol, int *toRow, int *toCol);
void getEmergencyMove(GameState* state, int *fromRow, int *fromCol, int *toRow, int *toCol);
void getAllValidMoves(GameState* state, int moves[][4], int *moveCount);
char getYesNoResponse();
bool parseAlgebraicNotation(GameState* state, const char* move, int* fromRow, int* fromCol, int* toRow, int* toCol);
bool isStalemate(GameState* state);
bool isInsufficientMaterial(GameState* state);
bool isThreefoldRepetition(GameState* state);
bool isFiftyMoveRule(GameState* state);
bool isDraw(GameState* state);
int moveValue(GameState* state, int move[4]);
int compareMove(const void* a, const void* b);
bool isValidCastling(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
void updateCastlingRights(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
int evaluateBoard(GameState* state);
int evaluatePawnStructure(GameState* state);
int evaluateKingSafety(GameState* state);
int evaluateThreats(GameState* state);
int getOpeningBookMove(GameState* state);
bool isPieceUnderAttack(GameState* state, int row, int col);
void moveCursor(char direction);
void printBoardWithCursor(GameState* state);
bool handleCursorSelection(GameState* state);
int minimax(GameState* state, int depth, int alpha, int beta, bool isMaximizingPlayer);
int quiescence(GameState* state, int alpha, int beta, bool isMaximizingPlayer);

// Piece values for evaluation
#define PAWN_VALUE 100
#define KNIGHT_VALUE 300
#define BISHOP_VALUE 300
#define ROOK_VALUE 500
#define QUEEN_VALUE 900

// Position value tables
const int pawnPositionValues[BOARD_SIZE][BOARD_SIZE] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

const int knightPositionValues[BOARD_SIZE][BOARD_SIZE] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

const int bishopPositionValues[BOARD_SIZE][BOARD_SIZE] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

const int rookPositionValues[BOARD_SIZE][BOARD_SIZE] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {0,  0,  0,  5,  5,  0,  0,  0}
};

const int queenPositionValues[BOARD_SIZE][BOARD_SIZE] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    {-5,  0,  5,  5,  5,  5,  0, -5},
    {0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};

const int kingPositionValues[BOARD_SIZE][BOARD_SIZE] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    {20, 20,  0,  0,  0,  0, 20, 20},
    {20, 30, 10,  0,  0, 10, 30, 20}
};

int main() {
    GameState state;
    bool playAgainstAI = false;
    bool playerIsWhite = true;
    
    printf("=== Chess Game for Ubuntu ===\n");
    printf("This chess game supports:\n");
    printf("- Human vs Human mode\n");
    printf("- Human vs AI mode with adjustable difficulty\n");
    printf("- Cursor-based piece movement (WASD + SPACE)\n");
    printf("- All standard chess rules including castling and en passant\n\n");
    
    srand(time(NULL)); // Initialize random seed for AI moves
    enableRawMode(); // Enable raw mode for immediate key capture

    printf("Do you want to play against the AI? (y/n): ");
    if (getYesNoResponse() == 'y') {
        playAgainstAI = true;
        printf("Do you want to play as White? (y/n): ");
        playerIsWhite = (getYesNoResponse() == 'y');
        printf("You will play as %s against the AI.\n", playerIsWhite ? "White" : "Black");
        
        // Prompt for AI depth
        disableRawMode(); // Temporarily disable for scanf
        do {
            printf("Enter the AI depth (1-6, higher depth means stronger but slower AI): ");
            scanf("%d", &aiDepth);
            if (aiDepth < 1 || aiDepth > 6) {
                printf("Invalid depth. Please enter a number between 1 and 6.\n");
            }
        } while (aiDepth < 1 || aiDepth > 6);
        enableRawMode(); // Re-enable raw mode
        
        printf("AI depth set to %d. Starting game...\n", aiDepth);
        sleepMs(1000);
    }
    
    initializeGameState(&state);
    
    while (true) {
        printBoardWithCursor(&state);
        
        if (isCheckmate(&state)) {
            printf("Checkmate! %s wins!\n", state.isWhiteTurn ? "Black" : "White");
            break;
        } else if (isStalemate(&state)) {
            printf("Stalemate! The game is a draw.\n");
            break;
        } else if (isDraw(&state)) {
            printf("Draw! Game ended due to draw conditions.\n");
            break;
        } else if (isInCheck(&state, state.isWhiteTurn)) {
            printf("%s is in check!\n", state.isWhiteTurn ? "White" : "Black");
        }

        if (playAgainstAI && state.isWhiteTurn != playerIsWhite) {
            printf("AI is thinking...\n");
            int fromRow, fromCol, toRow, toCol;
            getAIMove(&state, &fromRow, &fromCol, &toRow, &toCol);
            
            printf("AI wants to move: %c%d to %c%d\n", 'a' + fromCol, BOARD_SIZE - fromRow, 'a' + toCol, BOARD_SIZE - toRow);
            
            if (fromRow == -1 || fromCol == -1 || toRow == -1 || toCol == -1) {
                printf("AI returned invalid move! Game over!\n");
                break;
            }
            
            // Validate the AI move before making it
            if (!isValidMove(&state, fromRow, fromCol, toRow, toCol)) {
                printf("ERROR: AI suggested illegal move %c%d to %c%d!\n", 
                       'a' + fromCol, BOARD_SIZE - fromRow, 'a' + toCol, BOARD_SIZE - toRow);
                printf("Piece at source: '%c'\n", state.board[fromRow][fromCol]);
                printf("Piece at destination: '%c'\n", state.board[toRow][toCol]);
                printf("Current turn: %s\n", state.isWhiteTurn ? "White" : "Black");
                
                // Try emergency move instead
                getEmergencyMove(&state, &fromRow, &fromCol, &toRow, &toCol);
                if (fromRow == -1) {
                    printf("No valid moves available! Game over!\n");
                    break;
                }
                printf("Using emergency move: %c%d to %c%d\n", 
                       'a' + fromCol, BOARD_SIZE - fromRow, 'a' + toCol, BOARD_SIZE - toRow);
            }
            
            makeMove(&state, fromRow, fromCol, toRow, toCol);
            state.isWhiteTurn = !state.isWhiteTurn;
            printf("AI move executed: %c%d to %c%d\n", 'a' + fromCol, BOARD_SIZE - fromRow, 'a' + toCol, BOARD_SIZE - toRow);
            sleepMs(2000); // Give more time to see the move
        } else {
            printf("%s's turn. Use WASD to move cursor, SPACE to select/move, '%c' to quit: ", 
                   state.isWhiteTurn ? "White" : "Black", EXIT_KEY);
            char input = getch();
            if (input == EXIT_KEY) {
                printf("\nExiting the game. Thanks for playing!\n");
                break;
            } else if (input == ' ') {
                if (!handleCursorSelection(&state)) {
                    sleepMs(500); // Pause to show message
                }
            } else {
                moveCursor(input);
            }
        }
    }
    
    disableRawMode();
    return 0;
}

bool isPieceUnderAttack(GameState* state, int row, int col) {
    char piece = state->board[row][col];
    if (piece == EMPTY) return false;
    
    bool isWhitePiece = isupper(piece);

    // Check attacks from all enemy pieces
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char attackingPiece = state->board[i][j];
            if (attackingPiece != EMPTY &&
                ((isWhitePiece && islower(attackingPiece)) || (!isWhitePiece && isupper(attackingPiece)))) {
                
                // Temporarily set the turn to the attacking piece's color
                bool originalTurn = state->isWhiteTurn;
                state->isWhiteTurn = isupper(attackingPiece);
                
                if (isValidMove(state, i, j, row, col)) {
                    state->isWhiteTurn = originalTurn;
                    return true;
                }
                
                state->isWhiteTurn = originalTurn;
            }
        }
    }

    return false;
}

void initializeGameState(GameState* state) {
    printf("Initializing chess board with standard starting position...\n");
    
    // Initialize the board
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            state->board[i][j] = EMPTY;
        }
    }
    
    // Set up white pieces (bottom of board)
    state->board[7][0] = state->board[7][7] = WHITE_ROOK;
    state->board[7][1] = state->board[7][6] = WHITE_KNIGHT;
    state->board[7][2] = state->board[7][5] = WHITE_BISHOP;
    state->board[7][3] = WHITE_QUEEN;
    state->board[7][4] = WHITE_KING;
    for (int j = 0; j < BOARD_SIZE; j++) {
        state->board[6][j] = WHITE_PAWN;
    }
    
    // Set up black pieces (top of board)
    state->board[0][0] = state->board[0][7] = BLACK_ROOK;
    state->board[0][1] = state->board[0][6] = BLACK_KNIGHT;
    state->board[0][2] = state->board[0][5] = BLACK_BISHOP;
    state->board[0][3] = BLACK_QUEEN;
    state->board[0][4] = BLACK_KING;
    for (int j = 0; j < BOARD_SIZE; j++) {
        state->board[1][j] = BLACK_PAWN;
    }
    
    // Set initial game state
    state->isWhiteTurn = true;
    state->whiteCastleKingside = true;
    state->whiteCastleQueenside = true;
    state->blackCastleKingside = true;
    state->blackCastleQueenside = true;
    state->enPassantTargetRow = -1;
    state->enPassantTargetCol = -1;
    state->halfmoveClock = 0;
    state->fullmoveNumber = 1;
    state->canWhiteCastleKingside = true;
    state->canWhiteCastleQueenside = true;
    state->canBlackCastleKingside = true;
    state->canBlackCastleQueenside = true;
    
    sleepMs(500);
}

void printBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    printf("  a b c d e f g h\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%d ", BOARD_SIZE - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("%d\n", BOARD_SIZE - i);
    }
    printf("  a b c d e f g h\n");
}

bool isValidMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    // Boundary check
    if (fromRow < 0 || fromRow >= BOARD_SIZE || fromCol < 0 || fromCol >= BOARD_SIZE ||
        toRow < 0 || toRow >= BOARD_SIZE || toCol < 0 || toCol >= BOARD_SIZE) {
        return false;
    }
    
    char piece = state->board[fromRow][fromCol];
    
    // Check if there's a piece to move
    if (piece == EMPTY) return false;
    
    // Check if it's the correct player's turn
    if (state->isWhiteTurn && !isupper(piece)) return false;
    if (!state->isWhiteTurn && isupper(piece)) return false;
    
    // Check if trying to capture own piece
    if (state->isWhiteTurn && isupper(state->board[toRow][toCol])) return false;
    if (!state->isWhiteTurn && islower(state->board[toRow][toCol])) return false;
    
    // Check piece-specific move validity
    bool validPieceMove = false;
    switch (toupper(piece)) {
        case WHITE_PAWN:
            validPieceMove = isValidPawnMove(state, fromRow, fromCol, toRow, toCol);
            break;
        case WHITE_ROOK:
            validPieceMove = isValidRookMove(state, fromRow, fromCol, toRow, toCol);
            break;
        case WHITE_KNIGHT:
            validPieceMove = isValidKnightMove(state, fromRow, fromCol, toRow, toCol);
            break;
        case WHITE_BISHOP:
            validPieceMove = isValidBishopMove(state, fromRow, fromCol, toRow, toCol);
            break;
        case WHITE_QUEEN:
            validPieceMove = isValidQueenMove(state, fromRow, fromCol, toRow, toCol);
            break;
        case WHITE_KING:
            validPieceMove = isValidKingMove(state, fromRow, fromCol, toRow, toCol);
            break;
        default:
            return false;
    }
    
    if (!validPieceMove) return false;
    
    // Check if piece is pinned and move is not along pin ray
    PinInfo pinInfo = getPinInfo(state, fromRow, fromCol);
    if (pinInfo.isPinned && !isMoveAlongPinRay(fromRow, fromCol, toRow, toCol, pinInfo)) {
        return false;
    }
    
    // FIXED: Check if move puts own king in check (this was the bug in original code)
    if (doesMovePutKingInCheck(state, fromRow, fromCol, toRow, toCol)) {
        return false;
    }
    
    return true;
}

void makeMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[fromRow][fromCol];
    char capturedPiece = state->board[toRow][toCol];
    
    state->board[toRow][toCol] = state->board[fromRow][fromCol];
    state->board[fromRow][fromCol] = EMPTY;
    
    // Handle castling
    if (toupper(piece) == WHITE_KING && abs(fromCol - toCol) == 2) {
        int rookFromCol = (toCol > fromCol) ? 7 : 0;
        int rookToCol = (toCol > fromCol) ? toCol - 1 : toCol + 1;
        state->board[toRow][rookToCol] = state->board[fromRow][rookFromCol];
        state->board[fromRow][rookFromCol] = EMPTY;
        printf("Castling performed!\n");
    }
    
    // Handle en passant capture
    if (toupper(piece) == WHITE_PAWN && toCol == state->enPassantTargetCol && toRow == state->enPassantTargetRow) {
        state->board[fromRow][toCol] = EMPTY; // Remove the captured pawn
        printf("En passant capture!\n");
    }
    
    // Handle pawn promotion
    if ((piece == WHITE_PAWN && toRow == 0) || (piece == BLACK_PAWN && toRow == 7)) {
        promotePawn(state, toRow, toCol);
    }
    
    // Update castling rights
    updateCastlingRights(state, fromRow, fromCol, toRow, toCol);
    
    // Reset en passant target
    state->enPassantTargetRow = -1;
    state->enPassantTargetCol = -1;
    
    // Set en passant target for pawn double move
    if (toupper(piece) == WHITE_PAWN && abs(fromRow - toRow) == 2) {
        state->enPassantTargetRow = (fromRow + toRow) / 2;
        state->enPassantTargetCol = toCol;
    }
    
    // Update halfmove clock
    if (toupper(piece) == WHITE_PAWN || capturedPiece != EMPTY) {
        state->halfmoveClock = 0;
    } else {
        state->halfmoveClock++;
    }
    
    // Update fullmove number
    if (!state->isWhiteTurn) {
        state->fullmoveNumber++;
    }
}

void updateCastlingRights(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[toRow][toCol]; // Piece is already moved
    
    // Update castling rights based on piece moved
    if (piece == WHITE_KING) {
        state->canWhiteCastleKingside = false;
        state->canWhiteCastleQueenside = false;
    } else if (piece == BLACK_KING) {
        state->canBlackCastleKingside = false;
        state->canBlackCastleQueenside = false;
    } else if (piece == WHITE_ROOK) {
        if (fromRow == 7 && fromCol == 0) state->canWhiteCastleQueenside = false;
        if (fromRow == 7 && fromCol == 7) state->canWhiteCastleKingside = false;
    } else if (piece == BLACK_ROOK) {
        if (fromRow == 0 && fromCol == 0) state->canBlackCastleQueenside = false;
        if (fromRow == 0 && fromCol == 7) state->canBlackCastleKingside = false;
    }
    
    // Check if a rook is captured (affects castling rights)
    if (toRow == 0 && toCol == 0) state->canBlackCastleQueenside = false;
    if (toRow == 0 && toCol == 7) state->canBlackCastleKingside = false;
    if (toRow == 7 && toCol == 0) state->canWhiteCastleQueenside = false;
    if (toRow == 7 && toCol == 7) state->canWhiteCastleKingside = false;
}

bool isValidCastling(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[fromRow][fromCol];
    bool isWhite = isupper(piece);
    bool isKingSide = (toCol > fromCol);

    // Check if the piece is a king at starting position
    if (toupper(piece) != WHITE_KING || fromRow != (isWhite ? 7 : 0) || fromCol != 4) {
        return false;
    }

    // Check if the king is moving exactly two squares
    if (abs(toCol - fromCol) != 2 || toRow != fromRow) {
        return false;
    }

    // Check if the king is currently in check
    if (isInCheck(state, isWhite)) {
        return false;
    }

    // Check if castling rights are available
    if (isWhite) {
        if (isKingSide && !state->canWhiteCastleKingside) return false;
        if (!isKingSide && !state->canWhiteCastleQueenside) return false;
    } else {
        if (isKingSide && !state->canBlackCastleKingside) return false;
        if (!isKingSide && !state->canBlackCastleQueenside) return false;
    }

    // Check if the rook is still in position
    int rookCol = isKingSide ? 7 : 0;
    char expectedRook = isWhite ? WHITE_ROOK : BLACK_ROOK;
    if (state->board[fromRow][rookCol] != expectedRook) {
        return false;
    }

    // Check if the squares between the king and rook are empty
    int step = isKingSide ? 1 : -1;
    for (int col = fromCol + step; col != rookCol; col += step) {
        if (state->board[fromRow][col] != EMPTY) {
            return false;
        }
    }

    // Check if the king passes through or ends up in a square that's under attack
    // Include the king's starting square, path squares, and destination square
    int startCol = fromCol;
    int endCol = toCol;
    
    for (int col = startCol; col != endCol + step; col += step) {
        if (isSquareAttacked(state, fromRow, col, !isWhite)) {
            return false;
        }
    }

    return true;
}

bool isGameOver(GameState* state) {
    return isCheckmate(state) || isStalemate(state) || isDraw(state);
}

bool isValidPawnMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[fromRow][fromCol];
    int direction = isupper(piece) ? -1 : 1; // White moves up (-1), Black moves down (+1)
    int startRow = isupper(piece) ? 6 : 1;
    
    // Bounds check for target square
    if (toRow < 0 || toRow >= BOARD_SIZE || toCol < 0 || toCol >= BOARD_SIZE) {
        return false;
    }
    
    // Move forward one square
    if (fromCol == toCol && toRow == fromRow + direction && state->board[toRow][toCol] == EMPTY) {
        return true;
    }
    
    // Move forward two squares from starting position
    if (fromCol == toCol && fromRow == startRow && toRow == fromRow + 2 * direction) {
        // Check both squares are empty
        if (state->board[fromRow + direction][fromCol] == EMPTY && 
            state->board[toRow][toCol] == EMPTY) {
            return true;
        }
        return false;
    }
    
    // Capture diagonally
    if (abs(fromCol - toCol) == 1 && toRow == fromRow + direction && 
        state->board[toRow][toCol] != EMPTY && 
        ((isupper(piece) && islower(state->board[toRow][toCol])) || 
         (islower(piece) && isupper(state->board[toRow][toCol])))) {
        return true;
    }
    
    // Check for en passant
    if (abs(fromCol - toCol) == 1 && toRow == fromRow + direction &&
        state->board[toRow][toCol] == EMPTY &&
        toRow == state->enPassantTargetRow &&
        toCol == state->enPassantTargetCol) {
        return true;
    }
    
    return false;
}

bool isValidRookMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    // Rook moves horizontally or vertically only
    if (fromRow != toRow && fromCol != toCol) return false;
    
    // Can't move to same square
    if (fromRow == toRow && fromCol == toCol) return false;
    
    int rowStep = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
    int colStep = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
    
    int row = fromRow + rowStep;
    int col = fromCol + colStep;
    
    // Check if path is clear
    while (row != toRow || col != toCol) {
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
            return false; // Out of bounds
        }
        if (state->board[row][col] != EMPTY) return false;
        row += rowStep;
        col += colStep;
    }
    
    return true;
}

bool isValidKnightMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    int rowDiff = abs(toRow - fromRow);
    int colDiff = abs(toCol - fromCol);
    return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);
}

bool isValidBishopMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    // Bishop moves diagonally only
    if (abs(toRow - fromRow) != abs(toCol - fromCol)) return false;
    
    // Can't move to same square
    if (fromRow == toRow && fromCol == toCol) return false;
    
    int rowStep = (toRow > fromRow) ? 1 : -1;
    int colStep = (toCol > fromCol) ? 1 : -1;
    
    int row = fromRow + rowStep;
    int col = fromCol + colStep;
    
    // Check if path is clear
    while (row != toRow && col != toCol) {
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
            return false; // Out of bounds
        }
        if (state->board[row][col] != EMPTY) return false;
        row += rowStep;
        col += colStep;
    }
    
    return true;
}

bool isValidQueenMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    // Queen combines rook and bishop moves
    return isValidRookMove(state, fromRow, fromCol, toRow, toCol) || 
           isValidBishopMove(state, fromRow, fromCol, toRow, toCol);
}

bool isValidKingMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    // Normal king move (one square in any direction)
    if (abs(toRow - fromRow) <= 1 && abs(toCol - fromCol) <= 1) {
        return true;
    }

    // Check for castling (king moves two squares horizontally)
    if (abs(fromCol - toCol) == 2 && fromRow == toRow) {
        return isValidCastling(state, fromRow, fromCol, toRow, toCol);
    }

    return false;
}

bool isSquareAttacked(GameState* state, int row, int col, bool byWhite) {
    // Optimized attack detection with early exits
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = state->board[i][j];
            if (piece != EMPTY && ((byWhite && isupper(piece)) || (!byWhite && islower(piece)))) {
                // Temporarily change turn to check move validity
                bool originalTurn = state->isWhiteTurn;
                state->isWhiteTurn = byWhite;
                
                bool validMove = false;
                char pieceType = toupper(piece);
                
                // Optimized piece-specific attack checks
                switch (pieceType) {
                    case WHITE_PAWN: {
                        int direction = isupper(piece) ? -1 : 1;
                        if (abs(j - col) == 1 && row == i + direction) {
                            validMove = true; // Pawn captures diagonally
                        }
                        break;
                    }
                    case WHITE_ROOK:
                        if (i == row || j == col) {
                            validMove = isValidRookMove(state, i, j, row, col);
                        }
                        break;
                    case WHITE_KNIGHT:
                        validMove = isValidKnightMove(state, i, j, row, col);
                        break;
                    case WHITE_BISHOP:
                        if (abs(i - row) == abs(j - col)) {
                            validMove = isValidBishopMove(state, i, j, row, col);
                        }
                        break;
                    case WHITE_QUEEN:
                        if (i == row || j == col || abs(i - row) == abs(j - col)) {
                            validMove = isValidQueenMove(state, i, j, row, col);
                        }
                        break;
                    case WHITE_KING:
                        if (abs(i - row) <= 1 && abs(j - col) <= 1 && (i != row || j != col)) {
                            validMove = true;
                        }
                        break;
                }
                
                state->isWhiteTurn = originalTurn;
                
                if (validMove) {
                    return true; // Early exit when attack is found
                }
            }
        }
    }
    return false;
}

bool isInCheck(GameState* state, bool isWhiteKing) {
    int kingRow = -1, kingCol = -1;
    char kingPiece = isWhiteKing ? WHITE_KING : BLACK_KING;
    
    // Find the king's position
    for (int i = 0; i < BOARD_SIZE && kingRow == -1; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (state->board[i][j] == kingPiece) {
                kingRow = i;
                kingCol = j;
                break;
            }
        }
    }
    
    if (kingRow == -1 || kingCol == -1) {
        // King not found, this shouldn't happen in a valid game state
        return false;
    }
    
    return isSquareAttacked(state, kingRow, kingCol, !isWhiteKing);
}

bool isCheckmate(GameState* state) {
    bool isWhiteKing = state->isWhiteTurn;
    
    // Must be in check for checkmate
    if (!isInCheck(state, isWhiteKing)) {
        return false;
    }

    // Optimized checkmate detection: try king moves first, then blocking/capturing moves
    char kingPiece = isWhiteKing ? WHITE_KING : BLACK_KING;
    int kingRow = -1, kingCol = -1;
    
    // Find king position
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (state->board[i][j] == kingPiece) {
                kingRow = i;
                kingCol = j;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    // Try king escape moves first (most common escape)
    if (kingRow != -1) {
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                
                int newRow = kingRow + dr;
                int newCol = kingCol + dc;
                
                if (newRow >= 0 && newRow < BOARD_SIZE && 
                    newCol >= 0 && newCol < BOARD_SIZE) {
                    
                    if (isValidMove(state, kingRow, kingCol, newRow, newCol)) {
                        return false; // King can escape
                    }
                }
            }
        }
    }
    
    // Try all other pieces to see if they can block or capture
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            if ((isWhiteKing && isupper(piece)) || (!isWhiteKing && islower(piece))) {
                // Skip king as we already checked it
                if (toupper(piece) == WHITE_KING) continue;
                
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        if (isValidMove(state, fromRow, fromCol, toRow, toCol)) {
                            return false; // Found a valid move that might escape check
                        }
                    }
                }
            }
        }
    }

    return true; // No valid moves found to escape check
}

void promotePawn(GameState* state, int row, int col) {
    char promotion;
    
    disableRawMode(); // Temporarily disable raw mode for input
    
    do {
        printf("\nPawn promotion! Choose piece (Q/R/B/N): ");
        scanf(" %c", &promotion);
        promotion = toupper(promotion);
    } while (promotion != 'Q' && promotion != 'R' && promotion != 'B' && promotion != 'N');
    
    if (isupper(state->board[row][col])) {
        state->board[row][col] = promotion;
    } else {
        state->board[row][col] = tolower(promotion);
    }
    
    printf("Pawn promoted to %c!\n", promotion);
    
    enableRawMode(); // Re-enable raw mode
}

int evaluateBoard(GameState* state) {
    // Check cache first
    int cachedEval = getCachedEvaluation(state);
    if (cachedEval != INT_MIN) {
        return cachedEval;
    }
    
    int score = 0;
    int attackCount[2] = {0, 0}; // [white attacks, black attacks]
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            char piece = state->board[row][col];
            if (piece == EMPTY) continue;
            
            int pieceValue = 0;
            int positionValue = 0;
            bool isWhitePiece = isupper(piece);
            
            switch (toupper(piece)) {
                case WHITE_PAWN:
                    pieceValue = PAWN_VALUE;
                    positionValue = pawnPositionValues[row][col];
                    break;
                case WHITE_KNIGHT:
                    pieceValue = KNIGHT_VALUE;
                    positionValue = knightPositionValues[row][col];
                    break;
                case WHITE_BISHOP:
                    pieceValue = BISHOP_VALUE;
                    positionValue = bishopPositionValues[row][col];
                    break;
                case WHITE_ROOK:
                    pieceValue = ROOK_VALUE;
                    positionValue = rookPositionValues[row][col];
                    break;
                case WHITE_QUEEN:
                    pieceValue = QUEEN_VALUE;
                    positionValue = queenPositionValues[row][col];
                    break;
                case WHITE_KING:
                    pieceValue = 0; // King's value is not counted in material
                    positionValue = kingPositionValues[row][col];
                    break;
            }
            
            if (isWhitePiece) {
                score += pieceValue + positionValue;
                // Count attacks by this piece
                for (int tr = 0; tr < BOARD_SIZE; tr++) {
                    for (int tc = 0; tc < BOARD_SIZE; tc++) {
                        if (state->board[tr][tc] != EMPTY && islower(state->board[tr][tc])) {
                            bool originalTurn = state->isWhiteTurn;
                            state->isWhiteTurn = true;
                            if (isValidMove(state, row, col, tr, tc)) {
                                attackCount[0]++;
                            }
                            state->isWhiteTurn = originalTurn;
                        }
                    }
                }
                
                // Penalize if the piece is under attack
                if (isPieceUnderAttack(state, row, col)) {
                    score -= pieceValue / 3; // Increased penalty
                }
            } else {
                score -= pieceValue + positionValue;
                // Count attacks by this piece
                for (int tr = 0; tr < BOARD_SIZE; tr++) {
                    for (int tc = 0; tc < BOARD_SIZE; tc++) {
                        if (state->board[tr][tc] != EMPTY && isupper(state->board[tr][tc])) {
                            bool originalTurn = state->isWhiteTurn;
                            state->isWhiteTurn = false;
                            if (isValidMove(state, row, col, tr, tc)) {
                                attackCount[1]++;
                            }
                            state->isWhiteTurn = originalTurn;
                        }
                    }
                }
                
                // Reward if the opponent's piece is under attack
                if (isPieceUnderAttack(state, row, col)) {
                    score += pieceValue / 3; // Increased reward
                }
            }
        }
    }

    // Add bonus for controlling the center
    for (int row = 3; row <= 4; row++) {
        for (int col = 3; col <= 4; col++) {
            if (isupper(state->board[row][col])) score += 20;
            else if (islower(state->board[row][col])) score -= 20;
        }
    }

    // Add attack activity bonus
    score += (attackCount[0] - attackCount[1]) * 2;

    // Add pawn structure evaluation
    score += evaluatePawnStructure(state);

    // Add king safety evaluation
    score += evaluateKingSafety(state);

    // Immediate threat detection
    if (isInCheck(state, true)) {
        score -= 200; // Heavy penalty for white being in check
    }
    if (isInCheck(state, false)) {
        score += 200; // Bonus for putting black in check
    }

    // Scholar's Mate defense
    score += evaluateScholarsMateDefense(state);

    // Cache the evaluation
    cacheEvaluation(state, score);
    
    return score;
}

// COMPLETED: Previously empty function
int evaluatePawnStructure(GameState* state) {
    int score = 0;
    
    // Evaluate pawn chains, isolated pawns, doubled pawns, etc.
    for (int col = 0; col < BOARD_SIZE; col++) {
        int whitePawns = 0, blackPawns = 0;
        
        for (int row = 0; row < BOARD_SIZE; row++) {
            if (state->board[row][col] == WHITE_PAWN) whitePawns++;
            if (state->board[row][col] == BLACK_PAWN) blackPawns++;
        }
        
        // Penalize doubled pawns
        if (whitePawns > 1) score -= 10 * (whitePawns - 1);
        if (blackPawns > 1) score += 10 * (blackPawns - 1);
        
        // Check for isolated pawns
        bool whiteIsolated = true, blackIsolated = true;
        
        // Check adjacent columns for supporting pawns
        for (int adjCol = col - 1; adjCol <= col + 1; adjCol += 2) {
            if (adjCol >= 0 && adjCol < BOARD_SIZE) {
                for (int row = 0; row < BOARD_SIZE; row++) {
                    if (state->board[row][adjCol] == WHITE_PAWN) whiteIsolated = false;
                    if (state->board[row][adjCol] == BLACK_PAWN) blackIsolated = false;
                }
            }
        }
        
        if (whitePawns > 0 && whiteIsolated) score -= 15;
        if (blackPawns > 0 && blackIsolated) score += 15;
    }
    
    return score;
}

// COMPLETED: Previously empty function
int evaluateKingSafety(GameState* state) {
    int score = 0;
    int whiteKingRow = -1, whiteKingCol = -1;
    int blackKingRow = -1, blackKingCol = -1;
    
    // Find kings
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (state->board[i][j] == WHITE_KING) {
                whiteKingRow = i; whiteKingCol = j;
            }
            if (state->board[i][j] == BLACK_KING) {
                blackKingRow = i; blackKingCol = j;
            }
        }
    }
    
    // Evaluate pawn shield for white king
    if (whiteKingRow != -1 && whiteKingCol != -1) {
        for (int col = whiteKingCol - 1; col <= whiteKingCol + 1; col++) {
            if (col >= 0 && col < BOARD_SIZE) {
                if (whiteKingRow > 0 && state->board[whiteKingRow - 1][col] == WHITE_PAWN) {
                    score += 15; // Pawn in front of king
                }
                if (whiteKingRow > 1 && state->board[whiteKingRow - 2][col] == WHITE_PAWN) {
                    score += 10; // Pawn two squares in front
                }
            }
        }
    }
    
    // Evaluate pawn shield for black king
    if (blackKingRow != -1 && blackKingCol != -1) {
        for (int col = blackKingCol - 1; col <= blackKingCol + 1; col++) {
            if (col >= 0 && col < BOARD_SIZE) {
                if (blackKingRow < 7 && state->board[blackKingRow + 1][col] == BLACK_PAWN) {
                    score -= 15; // Pawn in front of king
                }
                if (blackKingRow < 6 && state->board[blackKingRow + 2][col] == BLACK_PAWN) {
                    score -= 10; // Pawn two squares in front
                }
            }
        }
    }
    
    // IMPROVED: Add immediate threat detection
    if (isInCheck(state, true)) {
        score -= 500; // Heavy penalty for being in check
    }
    if (isInCheck(state, false)) {
        score += 500;
    }
    
    // Penalize exposed kings in the opening/middlegame
    if (whiteKingRow != -1 && whiteKingCol != -1) {
        if (whiteKingRow >= 6) { // King still in back rank
            score += 50; // Good - king is safe
        } else {
            score -= 100; // Bad - king is exposed
        }
    }
    
    if (blackKingRow != -1 && blackKingCol != -1) {
        if (blackKingRow <= 1) { // King still in back rank
            score -= 50; // Good for black
        } else {
            score += 100; // Bad for black
        }
    }
    
    return score;
}

// NEW: Threat evaluation function to prevent tactical blunders
int evaluateThreats(GameState* state) {
    int score = 0;
    
    // Check for immediate tactical threats
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            char piece = state->board[row][col];
            if (piece == EMPTY) continue;
            
            bool isWhitePiece = isupper(piece);
            
            // Check if this piece is under attack
            if (isPieceUnderAttack(state, row, col)) {
                int pieceValue = 0;
                switch (toupper(piece)) {
                    case WHITE_PAWN: pieceValue = PAWN_VALUE; break;
                    case WHITE_KNIGHT: pieceValue = KNIGHT_VALUE; break;
                    case WHITE_BISHOP: pieceValue = BISHOP_VALUE; break;
                    case WHITE_ROOK: pieceValue = ROOK_VALUE; break;
                    case WHITE_QUEEN: pieceValue = QUEEN_VALUE; break;
                }
                
                if (isWhitePiece) {
                    score -= pieceValue / 2; // Penalize white pieces under attack
                } else {
                    score += pieceValue / 2; // Reward black pieces under attack
                }
            }
            
            // Check for fork threats (piece attacking multiple valuable targets)
            int attackCount = 0;
            for (int targetRow = 0; targetRow < BOARD_SIZE; targetRow++) {
                for (int targetCol = 0; targetCol < BOARD_SIZE; targetCol++) {
                    char targetPiece = state->board[targetRow][targetCol];
                    if (targetPiece != EMPTY && 
                        ((isWhitePiece && islower(targetPiece)) || (!isWhitePiece && isupper(targetPiece)))) {
                        
                        // Temporarily set turn to check if piece can attack target
                        bool originalTurn = state->isWhiteTurn;
                        state->isWhiteTurn = isWhitePiece;
                        
                        if (isValidMove(state, row, col, targetRow, targetCol)) {
                            attackCount++;
                        }
                        
                        state->isWhiteTurn = originalTurn;
                    }
                }
            }
            
            // Bonus for pieces that can attack multiple targets (forks)
            if (attackCount >= 2) {
                if (isWhitePiece) {
                    score += 30 * attackCount;
                } else {
                    score -= 30 * attackCount;
                }
            }
        }
    }
    
    // Check for discovered attack opportunities
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            char piece = state->board[row][col];
            if (piece == EMPTY) continue;
            
            // Check if moving this piece would reveal an attack
            for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                    if (isValidMove(state, row, col, toRow, toCol)) {
                        // Temporarily make the move
                        char tempPiece = state->board[toRow][toCol];
                        state->board[toRow][toCol] = state->board[row][col];
                        state->board[row][col] = EMPTY;
                        
                        // Check if this reveals an attack on a valuable piece
                        for (int targetRow = 0; targetRow < BOARD_SIZE; targetRow++) {
                            for (int targetCol = 0; targetCol < BOARD_SIZE; targetCol++) {
                                char targetPiece = state->board[targetRow][targetCol];
                                if (targetPiece != EMPTY && targetPiece != tempPiece) {
                                    bool isWhitePiece = isupper(piece);
                                    bool isWhiteTarget = isupper(targetPiece);
                                    
                                    if (isWhitePiece != isWhiteTarget) { // Different colors
                                        if (isPieceUnderAttack(state, targetRow, targetCol)) {
                                            int targetValue = 0;
                                            switch (toupper(targetPiece)) {
                                                case WHITE_PAWN: targetValue = PAWN_VALUE; break;
                                                case WHITE_KNIGHT: targetValue = KNIGHT_VALUE; break;
                                                case WHITE_BISHOP: targetValue = BISHOP_VALUE; break;
                                                case WHITE_ROOK: targetValue = ROOK_VALUE; break;
                                                case WHITE_QUEEN: targetValue = QUEEN_VALUE; break;
                                            }
                                            
                                            if (isWhitePiece) {
                                                score += targetValue / 4; // Bonus for white discovered attack
                                            } else {
                                                score -= targetValue / 4; // Penalty for black discovered attack
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // Undo the move
                        state->board[row][col] = state->board[toRow][toCol];
                        state->board[toRow][toCol] = tempPiece;
                    }
                }
            }
        }
    }
    
    return score;
}

// NEW: Simple opening book to prevent obvious blunders
int getOpeningBookMove(GameState* state) {
    printf("Checking opening book... Move %d, Turn: %s\n", 
           state->fullmoveNumber, state->isWhiteTurn ? "White" : "Black");
    
    // Check if we're in a standard opening position
    if (state->fullmoveNumber == 1) {
        // First move for white - prefer e4, d4, or Nf3
        if (state->isWhiteTurn) {
            // e4 (pawn from e2 to e4)
            if (state->board[6][4] == WHITE_PAWN && state->board[4][4] == EMPTY) {
                return 6444; // fromRow=6, fromCol=4, toRow=4, toCol=4
            }
            // d4 (pawn from d2 to d4)
            if (state->board[6][3] == WHITE_PAWN && state->board[4][3] == EMPTY) {
                return 6343; // fromRow=6, fromCol=3, toRow=4, toCol=3
            }
            // Nf3 (knight from g1 to f3)
            if (state->board[7][6] == WHITE_KNIGHT && state->board[5][5] == EMPTY) {
                return 7655; // fromRow=7, fromCol=6, toRow=5, toCol=5
            }
        } else {
            // First move for black - respond to common white openings
            // If white played e4, respond with e5
            if (state->board[4][4] == WHITE_PAWN && state->board[3][4] == EMPTY) {
                if (state->board[1][4] == BLACK_PAWN && state->board[3][4] == EMPTY) {
                    return 1434; // fromRow=1, fromCol=4, toRow=3, toCol=4
                }
            }
            // If white played d4, respond with d5
            if (state->board[4][3] == WHITE_PAWN) {
                if (state->board[1][3] == BLACK_PAWN && state->board[3][3] == EMPTY) {
                    return 1333; // fromRow=1, fromCol=3, toRow=3, toCol=3
                }
            }
            // If white played Nf3, respond with d5
            if (state->board[5][5] == WHITE_KNIGHT) {
                if (state->board[1][3] == BLACK_PAWN && state->board[3][3] == EMPTY) {
                    return 1333; // fromRow=1, fromCol=3, toRow=3, toCol=3
                }
            }
        }
    }
    
    // Second move for white
    if (state->fullmoveNumber == 2 && state->isWhiteTurn) {
        // If black played e5, develop knight to f3
        if (state->board[3][4] == BLACK_PAWN) {
            if (state->board[7][6] == WHITE_KNIGHT && state->board[5][5] == EMPTY) {
                return 7655; // Nf3
            }
        }
        // If black played d5, develop knight to c3
        if (state->board[3][3] == BLACK_PAWN) {
            if (state->board[7][1] == WHITE_KNIGHT && state->board[5][2] == EMPTY) {
                return 7152; // Nc3
            }
        }
    }
    
    // Second move for black
    if (state->fullmoveNumber == 2 && !state->isWhiteTurn) {
        // PRIORITY: Defend against Scholar's Mate - if white played Bc4, respond with Nf6
        if (state->board[4][2] == WHITE_BISHOP) {
            printf("Scholar's Mate threat detected! White has Bc4\n");
            if (state->board[0][6] == BLACK_KNIGHT && state->board[2][5] == EMPTY) {
                printf("Responding with Nf6 to defend f7!\n");
                return 625; // Nf6 - from (0,6) to (2,5)
            }
        }
        
        // If white played Nf3, develop knight to f6
        if (state->board[5][5] == WHITE_KNIGHT) {
            if (state->board[0][6] == BLACK_KNIGHT && state->board[2][5] == EMPTY) {
                return 625; // Nf6 - from (0,6) to (2,5)
            }
        }
        // If white played Nc3, develop knight to c6
        if (state->board[5][2] == WHITE_KNIGHT) {
            if (state->board[0][1] == BLACK_KNIGHT && state->board[2][2] == EMPTY) {
                return 122; // Nc6 - from (0,1) to (2,2)
            }
        }
    }
    
    // Third move for black - critical Scholar's Mate defense
    if (state->fullmoveNumber == 3 && !state->isWhiteTurn) {
        printf("Move 3 for Black - checking for Scholar's Mate threats\n");
        // If white threatens Scholar's Mate with Qh5, capture with knight
        if (state->board[3][7] == WHITE_QUEEN && state->board[2][5] == BLACK_KNIGHT) {
            printf("SCHOLAR'S MATE ATTEMPT! Capturing queen with Nxh5!\n");
            return 2537; // Nxh5 - capture the queen!
        }
        
        // If white hasn't played Qh5 yet but has Bc4, continue development with Nf6
        if (state->board[4][2] == WHITE_BISHOP && state->board[2][5] == EMPTY) {
            if (state->board[0][6] == BLACK_KNIGHT) {
                return 625; // Nf6 - from (0,6) to (2,5)
            }
        }
    }
    
    return -1; // No book move found
}

int quiescence(GameState* state, int alpha, int beta, bool isMaximizingPlayer) {
    int standPat = evaluateBoard(state);
    
    if (isMaximizingPlayer) {
        if (standPat >= beta) return beta;
        if (alpha < standPat) alpha = standPat;
    } else {
        if (standPat <= alpha) return alpha;
        if (beta > standPat) beta = standPat;
    }

    // Generate capturing moves only
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            if ((isMaximizingPlayer && isupper(piece)) || (!isMaximizingPlayer && islower(piece))) {
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        if (state->board[toRow][toCol] != EMPTY && 
                            ((isMaximizingPlayer && islower(state->board[toRow][toCol])) || 
                             (!isMaximizingPlayer && isupper(state->board[toRow][toCol])))) {
                            
                            if (isValidMove(state, fromRow, fromCol, toRow, toCol)) {
                                char tempPiece = state->board[toRow][toCol];
                                state->board[toRow][toCol] = state->board[fromRow][fromCol];
                                state->board[fromRow][fromCol] = EMPTY;

                                int score = quiescence(state, alpha, beta, !isMaximizingPlayer);

                                state->board[fromRow][fromCol] = state->board[toRow][toCol];
                                state->board[toRow][toCol] = tempPiece;

                                if (isMaximizingPlayer) {
                                    if (score >= beta) return beta;
                                    if (score > alpha) alpha = score;
                                } else {
                                    if (score <= alpha) return alpha;
                                    if (score < beta) beta = score;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return isMaximizingPlayer ? alpha : beta;
}

int minimax(GameState* state, int depth, int alpha, int beta, bool isMaximizingPlayer) {
    if (depth == 0) {
        return quiescence(state, alpha, beta, isMaximizingPlayer);
    }

    // Use improved move generation
    MoveList moveList;
    generateAllMoves(state, &moveList);
    
    if (moveList.count == 0) {
        return isInCheck(state, isMaximizingPlayer) ? -100000 : 0;
    }

    // Sort moves for better alpha-beta pruning
    for (int i = 0; i < moveList.count - 1; i++) {
        for (int j = i + 1; j < moveList.count; j++) {
            if (moveList.scores[j] > moveList.scores[i]) {
                // Swap moves
                for (int k = 0; k < 4; k++) {
                    int temp = moveList.moves[i][k];
                    moveList.moves[i][k] = moveList.moves[j][k];
                    moveList.moves[j][k] = temp;
                }
                int tempScore = moveList.scores[i];
                moveList.scores[i] = moveList.scores[j];
                moveList.scores[j] = tempScore;
            }
        }
    }

    int bestScore = isMaximizingPlayer ? -100000 : 100000;

    for (int i = 0; i < moveList.count; i++) {
        int fromRow = moveList.moves[i][0], fromCol = moveList.moves[i][1];
        int toRow = moveList.moves[i][2], toCol = moveList.moves[i][3];
        
        char tempPiece = state->board[toRow][toCol];
        state->board[toRow][toCol] = state->board[fromRow][fromCol];
        state->board[fromRow][fromCol] = EMPTY;

        int score = minimax(state, depth - 1, alpha, beta, !isMaximizingPlayer);

        state->board[fromRow][fromCol] = state->board[toRow][toCol];
        state->board[toRow][toCol] = tempPiece;

        if (isMaximizingPlayer) {
            bestScore = (score > bestScore) ? score : bestScore;
            alpha = (alpha > bestScore) ? alpha : bestScore;
        } else {
            bestScore = (score < bestScore) ? score : bestScore;
            beta = (beta < bestScore) ? beta : bestScore;
        }

        if (beta <= alpha) {
            break; // Alpha-beta pruning
        }
    }

    return bestScore;
}

int moveValue(GameState* state, int move[4]) {
    int fromRow = move[0], fromCol = move[1], toRow = move[2], toCol = move[3];
    char piece = state->board[fromRow][fromCol];
    char capturedPiece = state->board[toRow][toCol];
    
    int value = 0;
    
    // CRITICAL: Check if king is in check - prioritize defensive moves
    bool isWhitePiece = isupper(piece);
    if (isInCheck(state, isWhitePiece)) {
        // If king is in check, prioritize moves that get out of check
        char tempPiece = state->board[toRow][toCol];
        state->board[toRow][toCol] = state->board[fromRow][fromCol];
        state->board[fromRow][fromCol] = EMPTY;
        
        if (!isInCheck(state, isWhitePiece)) {
            value += 10000; // Very high priority for moves that escape check
        }
        
        state->board[fromRow][fromCol] = state->board[toRow][toCol];
        state->board[toRow][toCol] = tempPiece;
    }
    
    // Prioritize captures
    if (capturedPiece != EMPTY) {
        switch (toupper(capturedPiece)) {
            case WHITE_PAWN: value += PAWN_VALUE; break;
            case WHITE_KNIGHT: value += KNIGHT_VALUE; break;
            case WHITE_BISHOP: value += BISHOP_VALUE; break;
            case WHITE_ROOK: value += ROOK_VALUE; break;
            case WHITE_QUEEN: value += QUEEN_VALUE; break;
        }
    }
    
    // Prioritize pawn promotions
    if ((piece == WHITE_PAWN && toRow == 0) || (piece == BLACK_PAWN && toRow == 7)) {
        value += QUEEN_VALUE;
    }
    
    // Prioritize moving pieces out of danger
    if (isPieceUnderAttack(state, fromRow, fromCol)) {
        value += 100; // Increased from 50
    }
    
    // Prioritize defensive moves when king is threatened
    if (isInCheck(state, isWhitePiece)) {
        // Bonus for moves that block or capture attacking pieces
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                char attackingPiece = state->board[i][j];
                if (attackingPiece != EMPTY && 
                    ((isWhitePiece && islower(attackingPiece)) || (!isWhitePiece && isupper(attackingPiece)))) {
                    
                    // Check if this move captures the attacking piece
                    if (toRow == i && toCol == j) {
                        value += 500; // High bonus for capturing attacking piece
                    }
                    
                    // Check if this move blocks the attack
                    bool originalTurn = state->isWhiteTurn;
                    state->isWhiteTurn = isWhitePiece;
                    if (isValidMove(state, i, j, (isWhitePiece ? 7 : 0), 4)) { // King's position
                        // Check if our move blocks this attack
                        char tempPiece = state->board[toRow][toCol];
                        state->board[toRow][toCol] = state->board[fromRow][fromCol];
                        state->board[fromRow][fromCol] = EMPTY;
                        
                        if (!isValidMove(state, i, j, (isWhitePiece ? 7 : 0), 4)) {
                            value += 300; // Bonus for blocking attack
                        }
                        
                        state->board[fromRow][fromCol] = state->board[toRow][toCol];
                        state->board[toRow][toCol] = tempPiece;
                    }
                    state->isWhiteTurn = originalTurn;
                }
            }
        }
    }
    
    return value;
}

int compareMove(const void* a, const void* b) {
    return ((int*)b)[4] - ((int*)a)[4];
}

void getAIMove(GameState* state, int *fromRow, int *fromCol, int *toRow, int *toCol) {
    printf("AI thinking... Turn: %s, Move: %d\n", 
           state->isWhiteTurn ? "White" : "Black", state->fullmoveNumber);
    
    // IMPROVED: Check for opening book moves first
    if (state->fullmoveNumber <= 4) { // Only in early opening
        int bookMove = getOpeningBookMove(state);
        if (bookMove != -1) {
            // Parse book move (simplified format: fromRow*1000 + fromCol*100 + toRow*10 + toCol)
            *fromRow = bookMove / 1000;
            *fromCol = (bookMove % 1000) / 100;
            *toRow = (bookMove % 100) / 10;
            *toCol = bookMove % 10;
            printf("Opening book move: %c%d to %c%d\n", 
                   'a' + *fromCol, BOARD_SIZE - *fromRow,
                   'a' + *toCol, BOARD_SIZE - *toRow);
            
            // Validate the opening book move
            if (!isValidMove(state, *fromRow, *fromCol, *toRow, *toCol)) {
                printf("ERROR: Opening book suggested illegal move!\n");
                printf("Book move details: from(%d,%d) to(%d,%d)\n", *fromRow, *fromCol, *toRow, *toCol);
                printf("Piece at source: '%c'\n", state->board[*fromRow][*fromCol]);
                // Fall through to regular AI logic
            } else {
                return;
            }
        } else {
            printf("No opening book move found\n");
        }
    }
    
    int bestScore = state->isWhiteTurn ? -100000 : 100000;
    int bestFromRow = -1, bestFromCol = -1, bestToRow = -1, bestToCol = -1;
    
    // Use improved move generation system
    MoveList moveList;
    generateAllMoves(state, &moveList);
    
    printf("Generated %d moves\n", moveList.count);
    
    if (moveList.count == 0) {
        getEmergencyMove(state, fromRow, fromCol, toRow, toCol);
        return;
    }
    
    // Sort moves by their scores for better alpha-beta pruning
    for (int i = 0; i < moveList.count - 1; i++) {
        for (int j = i + 1; j < moveList.count; j++) {
            if (moveList.scores[j] > moveList.scores[i]) {
                // Swap moves
                for (int k = 0; k < 4; k++) {
                    int temp = moveList.moves[i][k];
                    moveList.moves[i][k] = moveList.moves[j][k];
                    moveList.moves[j][k] = temp;
                }
                int tempScore = moveList.scores[i];
                moveList.scores[i] = moveList.scores[j];
                moveList.scores[j] = tempScore;
            }
        }
    }
    
    // Debug: Show top 3 moves
    printf("Top moves: ");
    for (int i = 0; i < (moveList.count < 3 ? moveList.count : 3); i++) {
        printf("%c%d-%c%d(%d) ", 
               'a' + moveList.moves[i][1], BOARD_SIZE - moveList.moves[i][0],
               'a' + moveList.moves[i][3], BOARD_SIZE - moveList.moves[i][2],
               moveList.scores[i]);
    }
    printf("\n");
    
    // Search through sorted moves
    for (int i = 0; i < moveList.count; i++) {
        int fr = moveList.moves[i][0], fc = moveList.moves[i][1];
        int tr = moveList.moves[i][2], tc = moveList.moves[i][3];
        
        char tempPiece = state->board[tr][tc];
        state->board[tr][tc] = state->board[fr][fc];
        state->board[fr][fc] = EMPTY;

        int score = minimax(state, aiDepth - 1, -100000, 100000, !state->isWhiteTurn);

        state->board[fr][fc] = state->board[tr][tc];
        state->board[tr][tc] = tempPiece;

        if (state->isWhiteTurn) {
            if (score > bestScore) {
                bestScore = score;
                bestFromRow = fr; bestFromCol = fc;
                bestToRow = tr; bestToCol = tc;
            }
        } else {
            if (score < bestScore) {
                bestScore = score;
                bestFromRow = fr; bestFromCol = fc;
                bestToRow = tr; bestToCol = tc;
            }
        }
    }
    
    *fromRow = bestFromRow;
    *fromCol = bestFromCol;
    *toRow = bestToRow;
    *toCol = bestToCol;
    
    printf("AI chooses: %c%d to %c%d (score: %d)\n", 
           'a' + *fromCol, BOARD_SIZE - *fromRow,
           'a' + *toCol, BOARD_SIZE - *toRow, bestScore);
    
    // Emergency fallback if no move found
    if (*fromRow == -1) {
        getEmergencyMove(state, fromRow, fromCol, toRow, toCol);
    }
}

bool doesMovePutKingInCheck(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char tempPiece = state->board[toRow][toCol];
    state->board[toRow][toCol] = state->board[fromRow][fromCol];
    state->board[fromRow][fromCol] = EMPTY;

    bool kingInCheck = isInCheck(state, state->isWhiteTurn);

    // Undo the move
    state->board[fromRow][fromCol] = state->board[toRow][toCol];
    state->board[toRow][toCol] = tempPiece;

    return kingInCheck;
}

char getYesNoResponse() {
    char response;
    
    disableRawMode(); // Temporarily disable for scanf
    
    do {
        scanf(" %c", &response);
        response = tolower(response);
        if (response != 'y' && response != 'n') {
            printf("Please enter 'y' for yes or 'n' for no: ");
        }
    } while (response != 'y' && response != 'n');
    
    enableRawMode(); // Re-enable raw mode
    
    return response;
}

void moveCursor(char direction) {
    switch(direction) {
        case 'w': case 'W': // up
            cursorRow = (cursorRow - 1 + BOARD_SIZE) % BOARD_SIZE;
            break;
        case 's': case 'S': // down
            cursorRow = (cursorRow + 1) % BOARD_SIZE;
            break;
        case 'a': case 'A': // left
            cursorCol = (cursorCol - 1 + BOARD_SIZE) % BOARD_SIZE;
            break;
        case 'd': case 'D': // right
            cursorCol = (cursorCol + 1) % BOARD_SIZE;
            break;
    }
}

void printBoardWithCursor(GameState* state) {
    clearScreen();
    
    printf("\n=== Chess Game ===\n");
    printf("Turn: %s | Cursor: %c%d | Controls: WASD + SPACE\n\n", 
           state->isWhiteTurn ? "White" : "Black", 
           'a' + cursorCol, BOARD_SIZE - cursorRow);
    
    printf("   +---+---+---+---+---+---+---+---+\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf(" %d |", BOARD_SIZE - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = state->board[i][j];
            if (i == cursorRow && j == cursorCol) {
                if (pieceSelected && i == selectedRow && j == selectedCol) {
                    printf("<%c>|", piece == EMPTY ? ' ' : piece);
                } else {
                    printf("(%c)|", piece == EMPTY ? ' ' : piece);
                }
            } else if (pieceSelected && i == selectedRow && j == selectedCol) {
                printf("[%c]|", piece == EMPTY ? ' ' : piece);
            } else {
                printf(" %c |", piece == EMPTY ? ' ' : piece);
            }
        }
        printf(" %d\n", BOARD_SIZE - i);
        if (i < BOARD_SIZE - 1) {
            printf("   +---+---+---+---+---+---+---+---+\n");
        }
    }
    printf("   +---+---+---+---+---+---+---+---+\n");
    printf("     a   b   c   d   e   f   g   h\n\n");
    
    if (pieceSelected) {
        printf("Selected: %c%d - Move cursor to target and press SPACE\n", 
               'a' + selectedCol, BOARD_SIZE - selectedRow);
    }
}

bool handleCursorSelection(GameState* state) {
    if (!pieceSelected) {
        char piece = state->board[cursorRow][cursorCol];
        if ((state->isWhiteTurn && isupper(piece)) || (!state->isWhiteTurn && islower(piece))) {
            pieceSelected = true;
            selectedRow = cursorRow;
            selectedCol = cursorCol;
            printf("Piece selected at %c%d\n", 'a' + cursorCol, BOARD_SIZE - cursorRow);
            return true;
        } else {
            printf("No valid piece to select here!\n");
            return false;
        }
    } else {
        if (isValidMove(state, selectedRow, selectedCol, cursorRow, cursorCol)) {
            makeMove(state, selectedRow, selectedCol, cursorRow, cursorCol);
            state->isWhiteTurn = !state->isWhiteTurn;
            pieceSelected = false;
            selectedRow = -1;
            selectedCol = -1;
            printf("Move completed!\n");
            return true;
        } else if (cursorRow == selectedRow && cursorCol == selectedCol) {
            pieceSelected = false;
            selectedRow = -1;
            selectedCol = -1;
            printf("Selection cancelled.\n");
            return true;
        } else {
            printf("Invalid move! Try again.\n");
            return false;
        }
    }
}

bool isStalemate(GameState* state) {
    if (isInCheck(state, state->isWhiteTurn)) {
        return false; // Can't be stalemate if in check
    }

    // Check if any valid moves exist
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            if ((state->isWhiteTurn && isupper(piece)) || (!state->isWhiteTurn && islower(piece))) {
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        if (isValidMove(state, fromRow, fromCol, toRow, toCol)) {
                            return false; // Found a valid move
                        }
                    }
                }
            }
        }
    }

    return true; // No valid moves and not in check = stalemate
}

bool isInsufficientMaterial(GameState* state) {
    int whitePieces = 0, blackPieces = 0;
    int whiteKnights = 0, blackKnights = 0;
    int whiteBishops = 0, blackBishops = 0;
    int whiteBishopSquareColor = -1; // -1 = none, 0 = dark, 1 = light
    int blackBishopSquareColor = -1;
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = state->board[i][j];
            switch (piece) {
                case WHITE_PAWN: case WHITE_ROOK: case WHITE_QUEEN:
                    return false; // These pieces can deliver checkmate
                case BLACK_PAWN: case BLACK_ROOK: case BLACK_QUEEN:
                    return false;
                case WHITE_KNIGHT:
                    whiteKnights++; whitePieces++;
                    break;
                case BLACK_KNIGHT:
                    blackKnights++; blackPieces++;
                    break;
                case WHITE_BISHOP:
                    whiteBishops++; whitePieces++;
                    whiteBishopSquareColor = (i + j) % 2; // 0 for dark, 1 for light
                    break;
                case BLACK_BISHOP:
                    blackBishops++; blackPieces++;
                    blackBishopSquareColor = (i + j) % 2;
                    break;
                case WHITE_KING:
                    whitePieces++;
                    break;
                case BLACK_KING:
                    blackPieces++;
                    break;
            }
        }
    }
    
    // King vs King
    if (whitePieces == 1 && blackPieces == 1) return true;
    
    // King and Bishop vs King, or King and Knight vs King
    if ((whitePieces == 2 && blackPieces == 1 && (whiteBishops == 1 || whiteKnights == 1)) ||
        (blackPieces == 2 && whitePieces == 1 && (blackBishops == 1 || blackKnights == 1))) {
        return true;
    }
    
    // King and Bishop vs King and Bishop with bishops on same color
    if (whitePieces == 2 && blackPieces == 2 && 
        whiteBishops == 1 && blackBishops == 1 &&
        whiteBishopSquareColor == blackBishopSquareColor) {
        return true;
    }
    
    // King and Knight vs King and Knight
    if (whitePieces == 2 && blackPieces == 2 && 
        whiteKnights == 1 && blackKnights == 1) {
        return true;
    }
    
    return false;
}

bool isThreefoldRepetition(GameState* state) {
    // Simplified implementation - in a full game, you'd track position history
    return false;
}

bool isFiftyMoveRule(GameState* state) {
    return state->halfmoveClock >= 100; // 50 moves = 100 half-moves
}

bool isDraw(GameState* state) {
    return isStalemate(state) || isInsufficientMaterial(state) || isFiftyMoveRule(state) || isThreefoldRepetition(state);
} 
// Emergency fallback moves if AI gets stuck
void getEmergencyMove(GameState* state, int *fromRow, int *fromCol, int *toRow, int *toCol) {
    printf("Emergency move search for %s...\n", state->isWhiteTurn ? "White" : "Black");
    
    // Find any valid move for the current player
    for (int fr = 0; fr < BOARD_SIZE; fr++) {
        for (int fc = 0; fc < BOARD_SIZE; fc++) {
            char piece = state->board[fr][fc];
            if ((state->isWhiteTurn && isupper(piece)) || (!state->isWhiteTurn && islower(piece))) {
                for (int tr = 0; tr < BOARD_SIZE; tr++) {
                    for (int tc = 0; tc < BOARD_SIZE; tc++) {
                        if (isValidMove(state, fr, fc, tr, tc)) {
                            *fromRow = fr; *fromCol = fc; *toRow = tr; *toCol = tc;
                            printf("Emergency move found: %c%d to %c%d\n", 
                                   'a' + fc, BOARD_SIZE - fr, 'a' + tc, BOARD_SIZE - tr);
                            return;
                        }
                    }
                }
            }
        }
    }
    // If no valid moves, return invalid move
    printf("No emergency moves found!\n");
    *fromRow = -1; *fromCol = -1; *toRow = -1; *toCol = -1;
}

// Pin detection implementation
PinInfo getPinInfo(GameState* state, int row, int col) {
    PinInfo pinInfo = {false, {0, 0}, -1, -1};
    
    char piece = state->board[row][col];
    if (piece == EMPTY) return pinInfo;
    
    bool isWhitePiece = isupper(piece);
    char kingPiece = isWhitePiece ? WHITE_KING : BLACK_KING;
    
    // Find king position
    int kingRow = -1, kingCol = -1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (state->board[i][j] == kingPiece) {
                kingRow = i;
                kingCol = j;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    if (kingRow == -1) return pinInfo;
    
    // Check if piece is on the same ray as king
    int dr = kingRow - row;
    int dc = kingCol - col;
    
    // Normalize direction
    int stepR = 0, stepC = 0;
    if (dr != 0) stepR = dr / abs(dr);
    if (dc != 0) stepC = dc / abs(dc);
    
    // Check if it's a valid pin direction (straight or diagonal)
    if (!(dr == 0 || dc == 0 || abs(dr) == abs(dc))) {
        return pinInfo;
    }
    
    // Check if there's an attacking piece on the opposite side
    int checkRow = kingRow + stepR;
    int checkCol = kingCol + stepC;
    
    while (checkRow >= 0 && checkRow < BOARD_SIZE && 
           checkCol >= 0 && checkCol < BOARD_SIZE) {
        
        char checkPiece = state->board[checkRow][checkCol];
        if (checkPiece != EMPTY) {
            // Check if it's an enemy piece that can attack along this ray
            bool isEnemyPiece = (isWhitePiece && islower(checkPiece)) || 
                               (!isWhitePiece && isupper(checkPiece));
            
            if (isEnemyPiece) {
                // Check if this piece can attack along this direction
                char pieceType = toupper(checkPiece);
                bool canPin = false;
                
                if ((stepR == 0 || stepC == 0) && (pieceType == 'R' || pieceType == 'Q')) {
                    canPin = true; // Rook or Queen on rank/file
                }
                if ((abs(stepR) == abs(stepC)) && (pieceType == 'B' || pieceType == 'Q')) {
                    canPin = true; // Bishop or Queen on diagonal
                }
                
                if (canPin) {
                    pinInfo.isPinned = true;
                    pinInfo.pinDirection[0] = stepR;
                    pinInfo.pinDirection[1] = stepC;
                    pinInfo.pinningPieceRow = checkRow;
                    pinInfo.pinningPieceCol = checkCol;
                    return pinInfo;
                }
            }
            break; // Found a piece, stop searching in this direction
        }
        
        checkRow += stepR;
        checkCol += stepC;
    }
    
    return pinInfo;
}

bool isMoveAlongPinRay(int fromRow, int fromCol, int toRow, int toCol, PinInfo pin) {
    if (!pin.isPinned) return true;
    
    int moveDir[2] = {toRow - fromRow, toCol - fromCol};
    
    // Normalize move direction
    int moveDirNorm[2] = {0, 0};
    if (moveDir[0] != 0) moveDirNorm[0] = moveDir[0] / abs(moveDir[0]);
    if (moveDir[1] != 0) moveDirNorm[1] = moveDir[1] / abs(moveDir[1]);
    
    // Check if move is along the pin ray (same direction or opposite)
    return (moveDirNorm[0] == pin.pinDirection[0] && moveDirNorm[1] == pin.pinDirection[1]) ||
           (moveDirNorm[0] == -pin.pinDirection[0] && moveDirNorm[1] == -pin.pinDirection[1]);
}

// Generate all valid moves for current player
void generateAllMoves(GameState* state, MoveList* moveList) {
    moveList->count = 0;
    
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            
            // Check if piece belongs to current player
            if ((state->isWhiteTurn && isupper(piece)) || 
                (!state->isWhiteTurn && islower(piece))) {
                
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        if (isValidMove(state, fromRow, fromCol, toRow, toCol)) {
                            if (moveList->count < 256) {
                                moveList->moves[moveList->count][0] = fromRow;
                                moveList->moves[moveList->count][1] = fromCol;
                                moveList->moves[moveList->count][2] = toRow;
                                moveList->moves[moveList->count][3] = toCol;
                                
                                // Score move for ordering
                                moveList->scores[moveList->count] = 
                                    scoreMoveForOrdering(state, moveList->moves[moveList->count]);
                                
                                moveList->count++;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Score moves for better ordering in search
int scoreMoveForOrdering(GameState* state, int move[4]) {
    int fromRow = move[0], fromCol = move[1], toRow = move[2], toCol = move[3];
    int score = 0;
    
    char movingPiece = state->board[fromRow][fromCol];
    char capturedPiece = state->board[toRow][toCol];
    
    // Prioritize captures (MVV-LVA: Most Valuable Victim - Least Valuable Attacker)
    if (capturedPiece != EMPTY) {
        int victimValue = 0;
        int attackerValue = 0;
        
        switch (toupper(capturedPiece)) {
            case 'P': victimValue = 100; break;
            case 'N': case 'B': victimValue = 300; break;
            case 'R': victimValue = 500; break;
            case 'Q': victimValue = 900; break;
        }
        
        switch (toupper(movingPiece)) {
            case 'P': attackerValue = 100; break;
            case 'N': case 'B': attackerValue = 300; break;
            case 'R': attackerValue = 500; break;
            case 'Q': attackerValue = 900; break;
        }
        
        score += victimValue - attackerValue / 10;
    }
    
    // CRITICAL: Scholar's Mate defense has highest priority
    char piece = state->board[fromRow][fromCol];
    if (piece == BLACK_KNIGHT && toRow == 3 && toCol == 7) {
        // Nxh5 - capturing queen in Scholar's Mate
        score += 5000;
    }
    if (piece == BLACK_KNIGHT && toRow == 2 && toCol == 5) {
        // Nf6 - defending f7 square
        score += 1000;
    }
    
    // Prioritize checks
    char tempPiece = state->board[toRow][toCol];
    state->board[toRow][toCol] = state->board[fromRow][fromCol];
    state->board[fromRow][fromCol] = EMPTY;
    
    if (isInCheck(state, !state->isWhiteTurn)) {
        score += 50;
    }
    
    // Check if this move prevents Scholar's Mate
    if (!state->isWhiteTurn && isScholarsMateThreát(state)) {
        score += 2000; // Huge bonus for moves that counter Scholar's Mate
    }
    
    // Restore board
    state->board[fromRow][fromCol] = state->board[toRow][toCol];
    state->board[toRow][toCol] = tempPiece;
    
    // Prioritize center moves
    if ((toRow >= 3 && toRow <= 4) && (toCol >= 3 && toCol <= 4)) {
        score += 10;
    }
    
    // Prioritize moving pieces from attacked squares
    if (isPieceUnderAttack(state, fromRow, fromCol)) {
        score += 20;
    }
    
    return score;
}

// Cache management functions
int getCachedEvaluation(GameState* state) {
    for (int i = 0; i < 64; i++) {
        if (!evalCache[i].isValid) continue;
        
        if (evalCache[i].isWhiteTurn != state->isWhiteTurn) continue;
        
        // Compare board positions
        bool match = true;
        for (int row = 0; row < BOARD_SIZE && match; row++) {
            for (int col = 0; col < BOARD_SIZE && match; col++) {
                if (evalCache[i].board[row][col] != state->board[row][col]) {
                    match = false;
                }
            }
        }
        
        if (match) {
            return evalCache[i].evaluation;
        }
    }
    
    return INT_MIN; // Cache miss
}

void cacheEvaluation(GameState* state, int evaluation) {
    int index = cacheIndex % 64;
    cacheIndex++;
    
    evalCache[index].isWhiteTurn = state->isWhiteTurn;
    evalCache[index].evaluation = evaluation;
    evalCache[index].isValid = true;
    
    // Copy board state
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            evalCache[index].board[row][col] = state->board[row][col];
        }
    }
}

// Scholar's Mate and tactical threat detection
bool isScholarsMateThreát(GameState* state) {
    // Check if opponent is threatening Scholar's Mate (Qh5 + Bc4 targeting f7)
    bool hasQueenOnH5 = false;
    bool hasBishopOnC4 = false;
    
    // Look for white pieces threatening Scholar's Mate
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = state->board[i][j];
            if (piece == WHITE_QUEEN && i == 3 && j == 7) { // Qh5
                hasQueenOnH5 = true;
            }
            if (piece == WHITE_BISHOP && i == 4 && j == 2) { // Bc4
                hasBishopOnC4 = true;
            }
        }
    }
    
    return hasQueenOnH5 && hasBishopOnC4;
}

bool isMateThreateningMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[fromRow][fromCol];
    
    // Check if this move creates Scholar's Mate threat
    if (piece == WHITE_QUEEN && toRow == 3 && toCol == 7) { // Qh5
        // Check if Bc4 is already on board
        if (state->board[4][2] == WHITE_BISHOP) {
            return true; // This completes Scholar's Mate setup
        }
    }
    
    if (piece == WHITE_BISHOP && toRow == 4 && toCol == 2) { // Bc4
        // Check if Queen can threaten h5
        if (state->board[3][7] == WHITE_QUEEN) {
            return true; // This completes Scholar's Mate setup
        }
    }
    
    return false;
}

int evaluateScholarsMateDefense(GameState* state) {
    int score = 0;
    
    // Heavily reward defensive moves against Scholar's Mate
    if (!state->isWhiteTurn) { // Black's turn
        // Check if f7 square is defended
        if (state->board[2][5] == BLACK_KNIGHT) { // Nf6 defends f7
            score += 300;
        }
        
        // Check if Scholar's Mate threat exists
        if (isScholarsMateThreát(state)) {
            score -= 1000; // Heavy penalty for allowing mate threat
            
            // Reward capturing the threatening queen
            if (state->board[3][7] == WHITE_QUEEN) {
                // Check if knight on f6 can capture queen
                if (state->board[2][5] == BLACK_KNIGHT) {
                    score += 2000; // Huge bonus for capturing queen
                }
            }
        }
    }
    
    return score;
}
