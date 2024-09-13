#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <conio.h> 
#include <windows.h>

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

// Function prototypes
void initializeGameState(GameState* state);
void printBoard(char board[BOARD_SIZE][BOARD_SIZE]);
bool isValidMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
void makeMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol);
bool isGameOver(GameState* state);
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
bool isPieceUnderAttack(GameState* state, int row, int col);
// Add these global variables at the top of the file
int cursorRow = 0;
int cursorCol = 0;
bool pieceSelected = false;
int selectedRow = -1;
int selectedCol = -1;

// Add these function prototypes
void moveCursor(char direction);
void printBoardWithCursor(GameState* state);
bool handleCursorSelection(GameState* state);

// Increase the search depth for a stronger AI
#define MAX_DEPTH 5

// Add this global variable at the top of the file
int aiDepth = 5; // Default depth

int main() {
    GameState state;
    bool playAgainstAI = false;
    bool playerIsWhite = true;
    
    srand(time(NULL)); // Initialize random seed for AI moves

    printf("Do you want to play against the AI? (y/n): ");
    if (getYesNoResponse() == 'y') {
        playAgainstAI = true;
        printf("Do you want to play as White? (y/n): ");
        playerIsWhite = (getYesNoResponse() == 'y');
        printf("You will play as %s against the AI.\n", playerIsWhite ? "White" : "Black");
        
        // Prompt for AI depth
        do {
            printf("Enter the AI depth (1-10, higher depth means longer thinking time): ");
            scanf("%d", &aiDepth);
            if (aiDepth < 1 || aiDepth > 10) {
                printf("Invalid depth. Please enter a number between 1 and 10.\n");
            }
        } while (aiDepth < 1 || aiDepth > 10);
    }
    
    initializeGameState(&state);
    
    while (true) {
        printBoardWithCursor(&state);
        
        if (isCheckmate(&state)) {
            printf("Checkmate! %s wins!\n", state.isWhiteTurn ? "Black" : "White");
            break;
        } else if (isInCheck(&state, state.isWhiteTurn)) {
            printf("%s is in check!\n", state.isWhiteTurn ? "White" : "Black");
        }

        if (playAgainstAI && state.isWhiteTurn != playerIsWhite) {
            int fromRow, fromCol, toRow, toCol;
            getAIMove(&state, &fromRow, &fromCol, &toRow, &toCol);
            if (fromRow == -1 || fromCol == -1 || toRow == -1 || toCol == -1) {
                printf("Game over! %s wins!\n", playerIsWhite ? "White" : "Black");
                break;
            }
            makeMove(&state, fromRow, fromCol, toRow, toCol);
            state.isWhiteTurn = !state.isWhiteTurn;
            printf("AI move: %c%d to %c%d\n", 'a' + fromCol, BOARD_SIZE - fromRow, 'a' + toCol, BOARD_SIZE - toRow);
        } else {
            printf("%s's turn. Use WASD to move cursor, SPACE to select/move, '%c' to quit: ", state.isWhiteTurn ? "White" : "Black", EXIT_KEY);
            char input = _getch();
            if (input == EXIT_KEY) {
                printf("\nExiting the game. Thanks for playing!\n");
                break;
            } else if (input == ' ') {
                if (!handleCursorSelection(&state)) {
                    Sleep(1000); // Pause for a second to show the message
                }
            } else {
                moveCursor(input);
            }
        }
    }
    
    return 0;
}

// ... (implement the functions here)

bool isPieceUnderAttack(GameState* state, int row, int col) {
    char piece = state->board[row][col];
    bool isWhitePiece = isupper(piece);

    // Check attacks from all directions
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char attackingPiece = state->board[i][j];
            if ((isWhitePiece && islower(attackingPiece)) || (!isWhitePiece && isupper(attackingPiece))) {
                if (isValidMove(state, i, j, row, col)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void initializeGameState(GameState* state) {
    // Initialize the board
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            state->board[i][j] = EMPTY;
        }
    }
    
    // Set up white pieces
    state->board[7][0] = state->board[7][7] = WHITE_ROOK;
    state->board[7][1] = state->board[7][6] = WHITE_KNIGHT;
    state->board[7][2] = state->board[7][5] = WHITE_BISHOP;
    state->board[7][3] = WHITE_QUEEN;
    state->board[7][4] = WHITE_KING;
    for (int j = 0; j < BOARD_SIZE; j++) {
        state->board[6][j] = WHITE_PAWN;
    }
    
    // Set up black pieces
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
    if (fromRow < 0 || fromRow >= BOARD_SIZE || fromCol < 0 || fromCol >= BOARD_SIZE ||
        toRow < 0 || toRow >= BOARD_SIZE || toCol < 0 || toCol >= BOARD_SIZE) {
        return false;
    }
    
    char piece = state->board[fromRow][fromCol];
    
    if (state->isWhiteTurn && !isupper(piece)) return false;
    if (!state->isWhiteTurn && isupper(piece)) return false;
    
    if (state->isWhiteTurn && isupper(state->board[toRow][toCol])) return false;
    if (!state->isWhiteTurn && islower(state->board[toRow][toCol])) return false;
    
    switch (toupper(piece)) {
        case WHITE_PAWN:
            return isValidPawnMove(state, fromRow, fromCol, toRow, toCol);
        case WHITE_ROOK:
            return isValidRookMove(state, fromRow, fromCol, toRow, toCol);
        case WHITE_KNIGHT:
            return isValidKnightMove(state, fromRow, fromCol, toRow, toCol);
        case WHITE_BISHOP:
            return isValidBishopMove(state, fromRow, fromCol, toRow, toCol);
        case WHITE_QUEEN:
            return isValidQueenMove(state, fromRow, fromCol, toRow, toCol);
        case WHITE_KING:
            return isValidKingMove(state, fromRow, fromCol, toRow, toCol);
        default:
            return false;
    }
    if (doesMovePutKingInCheck(state, fromRow, fromCol, toRow, toCol)) {
        return false;
    }
    
    return true;
}
void makeMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[fromRow][fromCol];
    state->board[toRow][toCol] = state->board[fromRow][fromCol];
    state->board[fromRow][fromCol] = EMPTY;
    
    // Handle castling
    if (toupper(piece) == WHITE_KING && abs(fromCol - toCol) == 2) {
        int rookFromCol = (toCol > fromCol) ? 7 : 0;
        int rookToCol = (toCol > fromCol) ? toCol - 1 : toCol + 1;
        state->board[toRow][rookToCol] = state->board[fromRow][rookFromCol];
        state->board[fromRow][rookFromCol] = EMPTY;
    }
    
    // Handle en passant capture
    if (toupper(piece) == WHITE_PAWN && toCol == state->enPassantTargetCol && toRow == state->enPassantTargetRow) {
        state->board[fromRow][toCol] = EMPTY;
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
    if (piece == WHITE_PAWN || piece == BLACK_PAWN || state->board[toRow][toCol] != EMPTY) {
        state->halfmoveClock = 0;
    } else {
        state->halfmoveClock++;
    }
    
    // Update fullmove number
    if (!state->isWhiteTurn) {
        state->fullmoveNumber++;
    }
}
    // Update castling rights

void updateCastlingRights(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[fromRow][fromCol];
    
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
    
    // Check if a rook is captured
    if (toRow == 0 && toCol == 0) state->canBlackCastleQueenside = false;
    if (toRow == 0 && toCol == 7) state->canBlackCastleKingside = false;
    if (toRow == 7 && toCol == 0) state->canWhiteCastleQueenside = false;
    if (toRow == 7 && toCol == 7) state->canWhiteCastleKingside = false;
}

bool isValidCastling(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    char piece = state->board[fromRow][fromCol];
    bool isWhite = isupper(piece);
    bool isKingSide = (toCol > fromCol);

    // Check if the piece is a king and hasn't moved
    if (toupper(piece) != WHITE_KING || fromRow != (isWhite ? 7 : 0) || fromCol != 4) {
        return false;
    }

    // Check if the king is moving two squares
    if (abs(toCol - fromCol) != 2) {
        return false;
    }

    // Check if the king is in check
    if (isInCheck(state, isWhite)) {
        return false;
    }


    // Check if the squares between the king and rook are empty
    int rookCol = isKingSide ? 7 : 0;
    int step = isKingSide ? 1 : -1;
    for (int col = fromCol + step; col != rookCol; col += step) {
        if (state->board[fromRow][col] != EMPTY) {
            return false;
        }
    }

    // Check if the king passes through or ends up in check
    for (int col = fromCol; col != toCol + step; col += step) {
        GameState tempState = *state;
        tempState.board[fromRow][fromCol] = EMPTY;
        tempState.board[fromRow][col] = piece;
        if (isInCheck(&tempState, isWhite)) {
            return false;
        }
    }

    // Check if castling rights are available
    if (isWhite) {
        if (isKingSide && !state->canWhiteCastleKingside) return false;
        if (!isKingSide && !state->canWhiteCastleQueenside) return false;
    } else {
         if (isKingSide && !state->canBlackCastleKingside) return false;
        if (!isKingSide && !state->canBlackCastleQueenside) return false;
    }

    return true;
}


bool isGameOver(GameState* state) {
    return isCheckmate(state) || isStalemate(state) || isDraw(state);
}

// Implement specific piece movement rules
bool isValidPawnMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    int direction = state->isWhiteTurn ? -1 : 1;
    int startRow = state->isWhiteTurn ? 6 : 1;
    
    // Move forward one square
    if (fromCol == toCol && toRow == fromRow + direction && state->board[toRow][toCol] == EMPTY) {
        return true;
    }
    
    // Move forward two squares from starting position
    if (fromCol == toCol && fromRow == startRow && toRow == fromRow + 2 * direction &&
        state->board[fromRow + direction][fromCol] == EMPTY && state->board[toRow][toCol] == EMPTY) {
        return true;
    }
    
    // Capture diagonally
    if (abs(fromCol - toCol) == 1 && toRow == fromRow + direction && 
        state->board[toRow][toCol] != EMPTY && 
        ((state->isWhiteTurn && islower(state->board[toRow][toCol])) || (!state->isWhiteTurn && isupper(state->board[toRow][toCol])))) {
        return true;
    }
    
    // Check for en passant
    if (toupper(state->board[fromRow][fromCol]) == WHITE_PAWN && 
        abs(fromCol - toCol) == 1 && 
        state->board[toRow][toCol] == EMPTY &&
        toRow == state->enPassantTargetRow &&
        toCol == state->enPassantTargetCol) {
        return true;
    }
    
    return false;
}

bool isValidRookMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    if (fromRow != toRow && fromCol != toCol) return false;
    
    int rowStep = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
    int colStep = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
    
    int row = fromRow + rowStep;
    int col = fromCol + colStep;
    
    while (row != toRow || col != toCol) {
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
    if (abs(toRow - fromRow) != abs(toCol - fromCol)) return false;
    
    int rowStep = (toRow > fromRow) ? 1 : -1;
    int colStep = (toCol > fromCol) ? 1 : -1;
    
    int row = fromRow + rowStep;
    int col = fromCol + colStep;
    
    while (row != toRow && col != toCol) {
        if (state->board[row][col] != EMPTY) return false;
        row += rowStep;
        col += colStep;
    }
    
    return true;
}

bool isValidQueenMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    return isValidRookMove(state, fromRow, fromCol, toRow, toCol) || 
           isValidBishopMove(state, fromRow, fromCol, toRow, toCol);
}

bool isValidKingMove(GameState* state, int fromRow, int fromCol, int toRow, int toCol) {
    if (abs(toRow - fromRow) <= 1 && abs(toCol - fromCol) <= 1) {
        return true;
    }

    // Check for castling
    if (abs(fromCol - toCol) == 2 && fromRow == toRow) {
        return isValidCastling(state, fromRow, fromCol, toRow, toCol);
    }

    return false;
}

bool isKingCapturable(GameState* state, bool isWhiteKing) {
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
    
    // Create a temporary GameState to check the move
    GameState tempState;
    memcpy(tempState.board, state->board, sizeof(tempState.board));
    tempState.isWhiteTurn = !isWhiteKing;

    // Check if any opponent's piece can capture the king
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (state->board[i][j] != EMPTY && 
                ((isWhiteKing && islower(state->board[i][j])) || (!isWhiteKing && isupper(state->board[i][j])))) {
                if (isValidMove(&tempState, i, j, kingRow, kingCol)) {
                    return true;
                }
            }
        }
    }
    
    return false;
}
bool isSquareAttacked(GameState* state, int row, int col, bool byWhite) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = state->board[i][j];
            if ((byWhite && isupper(piece)) || (!byWhite && islower(piece))) {
                // Temporarily change turn to check move validity
                bool originalTurn = state->isWhiteTurn;
                state->isWhiteTurn = byWhite;
                bool validMove = isValidMove(state, i, j, row, col);
                state->isWhiteTurn = originalTurn;
                if (validMove) {
                    return true;
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
    
    if (!isInCheck(state, isWhiteKing)) {
        return false;
    }

    // Try all possible moves for the current player
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            if ((isWhiteKing && isupper(piece)) || (!isWhiteKing && islower(piece))) {
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        if (isValidMove(state, fromRow, fromCol, toRow, toCol)) {
                            // Make the move
                            char tempPiece = state->board[toRow][toCol];
                            state->board[toRow][toCol] = state->board[fromRow][fromCol];
                            state->board[fromRow][fromCol] = EMPTY;

                            // Check if the king is still in check
                                                        bool stillInCheck = isInCheck(state, isWhiteKing);

                            // Undo the move
                            state->board[fromRow][fromCol] = state->board[toRow][toCol];
                            state->board[toRow][toCol] = tempPiece;

                            if (!stillInCheck) {
                                return false; // Found a valid move that escapes check
                            }
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
    do {
        printf("Promote pawn to (Q/R/B/N): ");
        scanf(" %c", &promotion);
        promotion = toupper(promotion);
    } while (promotion != 'Q' && promotion != 'R' && promotion != 'B' && promotion != 'N');
    
    if (state->isWhiteTurn) {
        state->board[row][col] = promotion;
    } else {
        state->board[row][col] = tolower(promotion);
    }
}

// Piece values for evaluation
#define PAWN_VALUE 1
#define KNIGHT_VALUE 3
#define BISHOP_VALUE 3
#define ROOK_VALUE 5
#define QUEEN_VALUE 9

// Add these arrays at the top of your file, after the #define statements
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

// Update the evaluateBoard function
int evaluateBoard(GameState* state) {
    int score = 0;
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            char piece = state->board[row][col];
            int pieceValue = 0;
            int positionValue = 0;
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
                    pieceValue = 0; // King's value is not counted
                    positionValue = kingPositionValues[row][col];
                    break;
            }
            if (isupper(piece)) {
                score += pieceValue + positionValue;
                // Penalize if the piece is under attack
                if (isPieceUnderAttack(state, row, col)) {
                    score -= pieceValue / 2;
                }
            } else if (islower(piece)) {
                score -= pieceValue + positionValue;
                // Reward if the opponent's piece is under attack
                if (isPieceUnderAttack(state, row, col)) {
                    score += pieceValue / 2;
                }
            }
        }
    }

    // Add bonus for controlling the center
    for (int row = 3; row <= 4; row++) {
        for (int col = 3; col <= 4; col++) {
            if (isupper(state->board[row][col])) score += 10;
            else if (islower(state->board[row][col])) score -= 10;
        }
    }

    // Add bonus for pawn structure
    score += evaluatePawnStructure(state);

    // Add penalty for exposed king
    score += evaluateKingSafety(state);

    return score;
}

// Implement these new evaluation helper functions
int evaluatePawnStructure(GameState* state) {
    int score = 0;
    // Implement pawn structure evaluation logic here
    return score;
}

int evaluateKingSafety(GameState* state) {
    int score = 0;
    // Implement king safety evaluation logic here
    return score;
}

// Add this new function for quiescence search
int quiescence(GameState* state, int alpha, int beta, bool isMaximizingPlayer) {
    int standPat = evaluateBoard(state);
    if (isMaximizingPlayer) {
        if (standPat >= beta) return beta;
        if (alpha < standPat) alpha = standPat;
    } else {
        if (standPat <= alpha) return alpha;
        if (beta > standPat) beta = standPat;
    }

    int moves[BOARD_SIZE * BOARD_SIZE * BOARD_SIZE * BOARD_SIZE][5];
    int moveCount = 0;
    
    // Generate all capturing moves
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            if ((isMaximizingPlayer && isupper(piece)) || (!isMaximizingPlayer && islower(piece))) {
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        if (state->board[toRow][toCol] != EMPTY && 
                            ((isMaximizingPlayer && islower(state->board[toRow][toCol])) || 
                             (!isMaximizingPlayer && isupper(state->board[toRow][toCol])))) {
                            GameState tempState;
                            memcpy(tempState.board, state->board, sizeof(tempState.board));
                            tempState.isWhiteTurn = isMaximizingPlayer;
                            if (isValidMove(&tempState, fromRow, fromCol, toRow, toCol)) {
                                moves[moveCount][0] = fromRow;
                                moves[moveCount][1] = fromCol;
                                moves[moveCount][2] = toRow;
                                moves[moveCount][3] = toCol;
                                moves[moveCount][4] = moveValue(state, moves[moveCount]);
                                moveCount++;
                            }
                        }
                    }
                }
            }
        }
    }

    qsort(moves, moveCount, sizeof(moves[0]), compareMove);

    for (int i = 0; i < moveCount; i++) {
        int fromRow = moves[i][0], fromCol = moves[i][1], toRow = moves[i][2], toCol = moves[i][3];
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

    return isMaximizingPlayer ? alpha : beta;
}

// Update the minimax function to use quiescence search
int minimax(GameState* state, int depth, int alpha, int beta, bool isMaximizingPlayer) {
    if (depth == 0) {
        return quiescence(state, alpha, beta, isMaximizingPlayer);
    }

    int moves[BOARD_SIZE * BOARD_SIZE * BOARD_SIZE * BOARD_SIZE][5];
    int moveCount = 0;
    
    // Generate all valid moves
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            if ((state->isWhiteTurn && isupper(piece)) || (!state->isWhiteTurn && islower(piece))) {
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        GameState tempState;
                        memcpy(tempState.board, state->board, sizeof(tempState.board));
                        tempState.isWhiteTurn = state->isWhiteTurn;
                        if (isValidMove(&tempState, fromRow, fromCol, toRow, toCol)) {
                            moves[moveCount][0] = fromRow;
                            moves[moveCount][1] = fromCol;
                            moves[moveCount][2] = toRow;
                            moves[moveCount][3] = toCol;
                            moves[moveCount][4] = moveValue(state, moves[moveCount]);
                            moveCount++;
                        }
                    }
                }
            }
        }
    }
    
    if (moveCount == 0) {
        GameState tempState;
        memcpy(tempState.board, state->board, sizeof(tempState.board));
        tempState.isWhiteTurn = state->isWhiteTurn;
        return isInCheck(&tempState, state->isWhiteTurn) ? -1000000 : 0;
    }

    // Sort moves based on their value
    qsort(moves, moveCount, sizeof(moves[0]), compareMove);

    int bestScore = isMaximizingPlayer ? -1000000 : 1000000;

    for (int i = 0; i < moveCount; i++) {
        int fromRow = moves[i][0], fromCol = moves[i][1], toRow = moves[i][2], toCol = moves[i][3];
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
            break;
        }
    }

    return bestScore;
}

// Move ordering function
int moveValue(GameState* state, int move[4]) {
    int fromRow = move[0], fromCol = move[1], toRow = move[2], toCol = move[3];
    char piece = state->board[fromRow][fromCol];
    char capturedPiece = state->board[toRow][toCol];
    
    int value = 0;
    
    // Prioritize captures
    if (capturedPiece != EMPTY) {
        value = 10 * abs(evaluateBoard(state)) - abs(evaluateBoard(state));
    }
    
    // Prioritize pawn promotions
    if ((piece == WHITE_PAWN && toRow == 0) || (piece == BLACK_PAWN && toRow == 7)) {
        value += QUEEN_VALUE;
    }
    
    // Prioritize moving pieces out of danger
    if (isPieceUnderAttack(state, fromRow, fromCol) && !isPieceUnderAttack(state, toRow, toCol)) {
        value += 50;
    }
    
    return value;
}

int compareMove(const void* a, const void* b) {
    return ((int*)b)[4] - ((int*)a)[4];
}

void getAIMove(GameState* state, int *fromRow, int *fromCol, int *toRow, int *toCol) {
    int moves[BOARD_SIZE * BOARD_SIZE * BOARD_SIZE * BOARD_SIZE][5];
    int moveCount = 0;
    
    // Generate all valid moves with their values
    for (int fr = 0; fr < BOARD_SIZE; fr++) {
        for (int fc = 0; fc < BOARD_SIZE; fc++) {
            char piece = state->board[fr][fc];
            if ((state->isWhiteTurn && isupper(piece)) || (!state->isWhiteTurn && islower(piece))) {
                for (int tr = 0; tr < BOARD_SIZE; tr++) {
                    for (int tc = 0; tc < BOARD_SIZE; tc++) {
                        if (isValidMove(state, fr, fc, tr, tc) && !doesMovePutKingInCheck(state, fr, fc, tr, tc)) {
                            moves[moveCount][0] = fr;
                            moves[moveCount][1] = fc;
                            moves[moveCount][2] = tr;
                            moves[moveCount][3] = tc;
                            moves[moveCount][4] = moveValue(state, moves[moveCount]);
                            moveCount++;
                        }
                    }
                }
            }
        }
    }
    
    if (moveCount == 0) {
        printf("AI has no valid moves!\n");
        *fromRow = *fromCol = *toRow = *toCol = -1;
        return;
    }

    // Sort moves based on their value
    qsort(moves, moveCount, sizeof(moves[0]), compareMove);

    int bestScore = state->isWhiteTurn ? -1000000 : 1000000;
    int bestMoveIndex = 0;

    for (int i = 0; i < moveCount; i++) {
        char tempPiece = state->board[moves[i][2]][moves[i][3]];
        state->board[moves[i][2]][moves[i][3]] = state->board[moves[i][0]][moves[i][1]];
        state->board[moves[i][0]][moves[i][1]] = EMPTY;

        int score = minimax(state, aiDepth - 1, -1000000, 1000000, !state->isWhiteTurn);

        state->board[moves[i][0]][moves[i][1]] = state->board[moves[i][2]][moves[i][3]];
        state->board[moves[i][2]][moves[i][3]] = tempPiece;

        if (state->isWhiteTurn) {
            if (score > bestScore) {
                bestScore = score;
                bestMoveIndex = i;
            }
        } else {
            if (score < bestScore) {
                bestScore = score;
                bestMoveIndex = i;
            }
        }
    }

    *fromRow = moves[bestMoveIndex][0];
    *fromCol = moves[bestMoveIndex][1];
    *toRow = moves[bestMoveIndex][2];
    *toCol = moves[bestMoveIndex][3];
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
    do {
        scanf(" %c", &response);
        response = tolower(response);
        if (response != 'y' && response != 'n') {
            printf("Please enter 'y' for yes or 'n' for no: ");
        }
    } while (response != 'y' && response != 'n');
    return response;
}

void moveCursor(char direction) {
    switch(direction) {
        case 'w': // up
            cursorRow = (cursorRow - 1 + BOARD_SIZE) % BOARD_SIZE;
            break;
        case 's': // down
            cursorRow = (cursorRow + 1) % BOARD_SIZE;
            break;
        case 'a': // left
            cursorCol = (cursorCol - 1 + BOARD_SIZE) % BOARD_SIZE;
            break;
        case 'd': // right
            cursorCol = (cursorCol + 1) % BOARD_SIZE;
            break;
    }
}

void printBoardWithCursor(GameState* state) {
    system("cls");  // Clear screen (for Windows, use "clear" for Unix-based systems)
    
    printf("\n   +---+---+---+---+---+---+---+---+\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf(" %d |", BOARD_SIZE - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = state->board[i][j];
            if (i == cursorRow && j == cursorCol) {
                if (pieceSelected && i == selectedRow && j == selectedCol) {
                    printf("<%c>|", piece);
                } else {
                    printf("(%c)|", piece);
                }
            } else if (pieceSelected && i == selectedRow && j == selectedCol) {
                printf("[%c]|", piece);
            } else {
                printf(" %c |", piece);
              }
        }
        printf(" %d\n", BOARD_SIZE - i);
        if (i < BOARD_SIZE - 1) {
            printf("   +---+---+---+---+---+---+---+---+\n");
        }
    }
    printf("   +---+---+---+---+---+---+---+---+\n");
    printf("     a   b   c   d   e   f   g   h\n\n");
}

bool handleCursorSelection(GameState* state) {
    if (!pieceSelected) {
        char piece = state->board[cursorRow][cursorCol];
        if ((state->isWhiteTurn && isupper(piece)) || (!state->isWhiteTurn && islower(piece))) {
            pieceSelected = true;
            selectedRow = cursorRow;
            selectedCol = cursorCol;
        }
    } else {
        if (isValidMove(state, selectedRow, selectedCol, cursorRow, cursorCol)) {
            makeMove(state, selectedRow, selectedCol, cursorRow, cursorCol);
            state->isWhiteTurn = !state->isWhiteTurn;
            pieceSelected = false;
            selectedRow = -1;
            selectedCol = -1;
        } else if (cursorRow == selectedRow && cursorCol == selectedCol) {
            pieceSelected = false;
            selectedRow = -1;
            selectedCol = -1;
        } else {
            printf("Invalid move. Try again.\n");
        }
    }
    return false;
}

bool isStalemate(GameState* state) {
    if (isInCheck(state, state->isWhiteTurn)) {
        return false;
    }

    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            char piece = state->board[fromRow][fromCol];
            if ((state->isWhiteTurn && isupper(piece)) || (!state->isWhiteTurn && islower(piece))) {
                for (int toRow = 0; toRow < BOARD_SIZE; toRow++) {
                    for (int toCol = 0; toCol < BOARD_SIZE; toCol++) {
                        if (isValidMove(state, fromRow, fromCol, toRow, toCol)) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool isInsufficientMaterial(GameState* state) {
    int whitePieces = 0, blackPieces = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = state->board[i][j];
            if (isupper(piece)) {
                whitePieces++;
            } else if (islower(piece)) {
                blackPieces++;
            }
        }
    }

    return (whitePieces <= 1 && blackPieces <= 1);
}

bool isThreefoldRepetition(GameState* state) {
    // Implementation of threefold repetition rule is beyond the scope of this implementation
    return false;
}

bool isFiftyMoveRule(GameState* state) {
    return state->halfmoveClock >= 100;
}

bool isDraw(GameState* state) {
    return isStalemate(state) || isInsufficientMaterial(state) || isFiftyMoveRule(state);
}
