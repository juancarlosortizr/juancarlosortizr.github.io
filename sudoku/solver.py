"""
Note: Unfortunately, I couldn't find a way to run this inside the website.
So, I translated all this code into JavaScript and wrote it in the "solver.js" file.
Thus, this file doesn't actually provide any functionality to the sudoku solver
other than illustrating the DFS in Python, which, in my opinion, is easier to read
than in JS.
"""

import numpy as np

class State:
    """ Sudoku state class. """

    def __init__(self, board = np.full((9,9), None)):
        """
        board is the current state of the board. default input is all empty (Nones)
        cols maps a column index to the set of digits in that column
        rows maps a row index to the set of digits in that row
        boxes maps a box index (from (0,0) to (2,2)) to the set of digits in that 3x3 box
        """
        self.board = board
        self.rows = {row: set() for row in range(9)}
        self.cols = {col: set() for col in range(9)}
        self.boxes = {coord1: {coord2: set() for coord2 in range(3)} for coord1 in range(3)}
        
        for i in range(9):
            for j in range(9):
                if self.board[i,j] != None:
                    digit = self.board[i,j]
                    self.rows[i].add(digit)
                    self.cols[j].add(digit)
                    self.boxes[i//3][j//3].add(digit)

        return

    def get_board(self):
        # Return current state of board
        return self.board

    def get_square(self, i, j):
        # Return digit in square (i,j), or None if no digit is present there
        return self.board[i,j]

    def is_valid(self, digit, i, j):
        # Returns True if board state is valid after adding digit to the empty square (i,j)
        return (self.board[i,j] == None and \
                (digit not in self.rows[i]) and \
                (digit not in self.cols[j]) and \
                (digit not in self.boxes[i//3][j//3]))

    def set_square(self, digit, i, j):
    # Set digit in square (i,j) to be given digit, only if result is still valid
    # If not, raises a ValueError
        if not self.is_valid(digit, i, j):
            raise ValueError("Inserting this digit here results in a bad sudoku!")
        self.board[i,j] = digit
        self.rows[i].add(digit)
        self.cols[j].add(digit)
        self.boxes[i//3][j//3].add(digit)

    def forget_square(self, i, j):
        # Forget digit in square (i,j)
        digit = self.board[i,j]
        self.rows[i].remove(digit)
        self.cols[j].remove(digit)
        self.boxes[i//3][j//3].remove(digit)
        self.board[i,j] = None

class Solver:
    """ Solves the sudoku board. """
    
    def __init__(self, board = np.full((9,9), None)):
        self.state = State(board)

    def dfs(self, i, j):
        """ Uses DFS to solve, starting at square (i,j). Goes row by row. """
        
        # First, check if we've finished the current row.
        if j==9:
            j = 0
            i += 1

        # Next, check if we've finished all rows.
        if i==9:
            return True

        # If the square is already filled in, move on to the next one.
        if self.state.get_square(i,j) != None:
            return self.dfs(i, j+1)

        # Else, check filling it in with 1 thru 9
        for digit in range(1,10):
            if self.state.is_valid(digit, i, j):
                self.state.set_square(digit, i, j)

                # Check if this branch of the DFS finds a valid solution
                if self.dfs(i, j+1):
                    return True

                # Otherwise if no valid solution is found, backtrack.
                self.state.forget_square(i, j)
        
        # If no digit fits in this square, then the sudoku is unsolvable at this point.
        return False

    def get_board(self):
        # Return current state of board
        return self.state.get_board()

def solve_empty_board():
    # Solve an empty board
    empty = Solver()
    if empty.dfs(0,0):
        return empty.get_board()
    return "Board cannot be solved!"

def solve_board(board):
    # Solve a given np.array 9x9 board
    solver = Solver(board)
    if solver.dfs(0,0):
        return solver.get_board()
    return "Board cannot be solved!"

def user_input_board():
    board = np.full((9,9), None)
    for i in range(9):
        for j in range(9):
            while True:
                digit = input("Input the digit in square ("+str(i)+","+str(j)+") ('.' will skip the square): ")
                if digit in [str(d) for d in range(1,10)]:
                    board[i,j] = int(digit)
                    break
                elif digit == '.':
                    break
                else:
                    print('Not a valid digit! Try again.')
    return board

def main():
    # Trying out the solver on an empty board
    print(solve_empty_board())

    # Trying it out on a board the user inputs. 
    board = user_input_board()
    print('Here is the solution to your board:')
    print(solve_board(board))
    
if __name__ == "__main__":
    main()