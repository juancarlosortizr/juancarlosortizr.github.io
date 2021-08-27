function solveSudoku() {
    var board = new Array(81);
    const digits = ['1','2','3','4','5','6','7','8','9'];
    for (let i = 0; i < 81; i++) {
        var idName = 'x'+(i+1).toString();
        var x = document.getElementById(idName).value;
        if (digits.includes(x)) {board[i] = x;}
        else {board[i] = ".";}
    }

    // use solver.js to convert answer into the actual answer.
    // If no solution, then it's just full of dots.
    board = solveBoard(board);

    for (let i = 0; i < 81; i++) {
        var idName = 'board'+(i+1).toString();
        document.getElementById(idName).innerHTML = board[i].toString();
    } 
}

// The following function is the brain of the whole thing.
// It is translated from solver.py, which in my opinion is easier to read.

function solveBoard(board){
    // Input a board (in 81-array string form, instead of the more intuitie 9x9 form)
    // Onput a board (in 81-array int form), that is full of "." if there is no solution
    // Or, if a solution does exist, output a board that is a solution. (Not necessarily unique)

    intBoard = new Array(81);
    for (let i=0; i<81; i++) {
        if (board[i] == ".") {
            intBoard[i] = ".";
        }
        else {
            intBoard[i] = parseInt(board[i]);
        }
    }
    var solver = new Solver(intBoard);
    
    if(!solver.state.is_currently_valid()) {
        alert('The sudoku you inputted has violations!');
        newBoard = new Array(81).fill(".");
        return newBoard;
    }
    else {
        //alert("The sudoku you inputted doesn't have violations!");
    }

    if (solver.dfs(0,0)) {
        alert('A solution exists! It is:');
        solvedBoard = solver.get_board();
        //alert(solvedBoard);
        newBoard = new Array(81).fill(".");
        for (let i=0; i<81; i++) {
            newBoard[i] = solvedBoard[Math.floor(i/9)][i - 9*Math.floor(i/9)];
        }
        return newBoard;
    }

    // If no solution exists, return a board full of dots.
    alert('No solution exists!')
    newBoard = new Array(81).fill(".");
    return newBoard;
}

class State{

    constructor(board) {
        // Initiate board, rows, cols, and boxes
        // board must be 81-array int form.
        this.board = new Array(9);
        for (let i=0; i<9; i++) {
            this.board[i] = new Array(9);
        }
        this.rows = new Array(9);
        this.cols = new Array(9);
        this.boxes = new Array(3);
        for (let i=0; i<3; i++) {
            this.boxes[i] = new Array(3);
        }
        
        for (let i=0; i<9; i++) {
            this.rows[i] = new Set();
            this.cols[i] = new Set();
        }

        for (let i=0; i<3; i++) {
            for (let j=0; j<3; j++) {
                this.boxes[i][j] = new Set();
            }
        }

        // read board into this.board
        for (let i=0;i<9;i++) {
            for (let j=0;j<9;j++) {
                this.board[i][j] = board[i*9+j];
            }
        }

        // Compute rows, cols, and boxes
        for (let i=0;i<9;i++) {
            for (let j=0;j<9;j++) {
                if (this.board[i][j] != ".") {
                    var digit = this.board[i][j];
                    this.rows[i].add(digit);
                    this.cols[j].add(digit);
                    this.boxes[Math.floor(i/3)][Math.floor(j/3)].add(digit);
                }
            }
        }
    }

    get_board() {
        // Return current state of board
        return this.board;
    }

    get_square(i,j) {
        // Return digit in square (i,j), or "." if no digit is present there
        return this.board[i][j];
    }

    is_valid(digit, i, j) {
        // Returns true if board state is valid after adding digit to the empty square (i,j)
        return (this.board[i][j] == "." &&
                !(this.rows[i].has(digit)) &&
                !(this.cols[j].has(digit)) &&
                !(this.boxes[Math.floor(i/3)][Math.floor(j/3)].has(digit)));
    }

    is_currently_valid() {
        //alert("Checking validity");
        // Returns true if board is currently valid

        // Check rows don't have repeat
        for (let i=0; i<9; i++) {
            var currRow = new Set();
            for (let j=0; j<9; j++) {
                if (currRow.has(this.board[i][j])) {
                    return false;
                }
                if (this.board[i][j] != ".") {
                    currRow.add(this.board[i][j]);
                }
            }
        }

        //alert("rows valid");

        // Check columns don't have repeat
        for (let j=0; j<9; j++) {
            var currCol = new Set();
            for (let i=0; i<9; i++) {
                if (currCol.has(this.board[i][j])) {
                    return false;
                }
                if (this.board[i][j] != ".") {
                    currCol.add(this.board[i][j]);
                }
            }
        }


        //alert("cols valid");

        //  Check boxes don't have repeat
        for (let i=0; i<3; i++) {
            for (let j=0; j<3; j++) {
                var currBox = new Set();
                for (let a=0; a<9; a++) {
                    var currDigit = this.board[i+Math.floor(a/3)][j+(a-3*Math.floor(a/3))];
                    if (currBox.has(currDigit)) {
                        return false;
                    }
                    if (currDigit != ".") {
                        currBox.add(currDigit);
                    }
                }
            }
        }


        //alert("boxes valid");

        return true;
    }

    set_square(digit, i, j) {
        // Set digit in square (i,j) to be given digit, only if result is still valid
        // If not, throws an error
        if (!this.is_valid(digit,i,j)) {
            alert("Inserting digit here results in a bad sudoku!");
            throw "Inserting digit here results in a bad sudoku!";
        }
        this.board[i][j] = digit;
        this.rows[i].add(digit);
        this.cols[j].add(digit);
        this.boxes[Math.floor(i/3)][Math.floor(j/3)].add(digit);
    }

    forget_square(i,j) {
        // Forget digit in square (i,j)
        //alert('Forget1');
        var myDigit = this.board[i][j];
        //alert('Forget2');
        this.rows[i].delete(myDigit);
        //alert('Forget3');
        this.cols[j].delete(myDigit);
        //alert('Forget4');
        this.boxes[Math.floor(i/3)][Math.floor(j/3)].delete(myDigit);
        //alert('Forget5');
        this.board[i][j] = ".";
        //alert('Forget6');
    }

}

class Solver {
    // Solves the sudoku board

    constructor(board) {
        this.state = new State(board);
    }

    dfs(i,j) {
        //alert('new dfs '+i.toString()+","+j.toString());
        // Uses DFS to solve, starting at square (i,j)
        // Goes row by row

        // First, check if we've finished current row.
        if (j==9) {
            return this.dfs(i+1,0);
        }

        // Next, check if we've finished all rows
        if (i==9) {
            return true;
        }

        // If the square is already filled, 
        // move on to the next one
        if (this.state.get_square(i,j) != ".") {
            return this.dfs(i, j+1);
        }

        if (i==1 && j==6) {
            //alert(this.get_board());
        }
        // Else, check filling it in with 1 thru 9
        for (let digit=1; digit<10; digit++) {

            if (i==1 && j==6) {
                //alert('Trying '+digit.toString());
            }

            if (this.state.is_valid(digit,i,j)) {

                if (i==1 && j==6) {
                    //alert('VALID DIGIT WITH 1,6!');
                }

                this.state.set_square(digit,i,j);

                // Check if this branch finds a valid solution
                if (this.dfs(i,j+1)) {
                    if (i==0 && j==0) {
                        //alert("Returning true on 0,0!");
                    }
                    return true;
                }

                // Otherwise backtrack
                //alert("Backtracking on"+i.toString()+","+j.toString()+" with digit "+this.state.get_square(i,j).toString());
                this.forget_square(i,j);
                //alert("FORGET WORKS");
                //alert(this.state.get_square(i,j));
            }

            if (i==1 && j==6) {
                //alert("Digit failed on 1,6");
            }

        }

        // If no digit works, then sudoku is unsolvable
        if (i==0 && j==0) {
            //alert("Returning false on 0,0!");
        }

        if (i==1 && j==6) {
            //alert("Failed on "+i.toString()+","+j.toString());
        }

        return false;
    }

    get_board() {
        // Return current state of board
        return this.state.get_board();
    }

    forget_square(i,j) {
        this.state.forget_square(i,j);
    }
}