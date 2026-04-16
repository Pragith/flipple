let dialog = require("dialog");
let math = require("math");

// Predefined list of words
let words = ["during", "return", "events", "little", "change"];
let words3 = ["DURING", "RETURN", "EVENTS", "LITTLE", "CHANGE"];

// Function to get a random word from the list
function getRandomWord() {
    return words[math.floor(math.random() * words.length)];
}

// Function to generate a random letter
function getRandomLetter() {
    let letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return letters[math.floor(math.random() * letters.length)];
}

// Function to create the grid
function createGrid(targetWord) {
    let grid = [];
    for (let i = 0; i < targetWord.length; i++) {
        let columnLength = math.floor(math.random() * 5) + 3; // Column length between 3 and 7
        let column = [];
        for (let j = 0; j < columnLength; j++) {
            column.push(getRandomLetter());
        }
        // Ensure the target letter is in the column
        let position = math.floor(math.random() * columnLength);
        column[position] = targetWord[i];
        grid.push(column);
    }
    return grid;
}

// Function to display the grid
function displayGrid(grid, rowIndex, colIndex) {
    let gridDisplay = "";
    for (let i = 0; i < grid.length; i++) {
        for (let j = 0; j < grid[i].length; j++) {
            if (i === rowIndex && j === colIndex) {
                gridDisplay += "[" + grid[i][j] + "] ";
            } else if (j === math.floor(grid[i].length / 2)) {
                gridDisplay += "[" + grid[i][j] + "] ";
            } else {
                gridDisplay += " " + grid[i][j] + "  ";
            }
        }
        gridDisplay += "\n";
    }
    return gridDisplay;
}

// Function to get the user's guess
function getUserGuess(grid, attempts) {
    let selectedWord = [];
    let colIndex = math.floor(grid[0].length / 2); // Center column
    let rowIndex = 0;
    let maxRow = grid.length;
    let maxCol = grid[0].length;
    let lastAction = "";
    let exitCount = 0;

    while (selectedWord.length < grid.length) {
        let columnDisplay = displayGrid(grid, rowIndex, colIndex);
        let action = dialog.custom({
            header: "Flipple",
            text: "Remaining Turns: x" + to_string(attempts) + '\n' + columnDisplay,
            button_left: "<",
            button_right: ">",
            button_up: "^",
            button_down: "v",
            button_center: "OK",
            button_back: "back"
        });

        if (action === "back") {
            if (lastAction === "back") {
                exitCount++;
            } else {
                exitCount = 0;
            }
            lastAction = "back";
            if (exitCount >= 2) {
                dialog.message("Exiting", "You have exited the game.");
                return null; // Exit the program
            }
        } else {
            exitCount = 0;
            if (action === ">") {
                // Manually shift elements to the left
                let temp = grid[rowIndex][0];
                for (let i = 0; i < grid[rowIndex].length - 1; i++) {
                    grid[rowIndex][i] = grid[rowIndex][i + 1];
                }
                grid[rowIndex][grid[rowIndex].length - 1] = temp;
            } else if (action === "<") {
                // Manually shift elements to the right
                let temp = grid[rowIndex][grid[rowIndex].length - 1];
                for (let i = grid[rowIndex].length - 1; i > 0; i--) {
                    grid[rowIndex][i] = grid[rowIndex][i - 1];
                }
                grid[rowIndex][0] = temp;
            } else if (action === "^" && rowIndex > 0) {
                rowIndex--;
            } else if (action === "v" && rowIndex < maxRow - 1) {
                rowIndex++;
            } else if (action === "OK") {
                selectedWord.push(grid[rowIndex][colIndex]);
                rowIndex++;
                colIndex = math.floor(grid[0].length / 2);
            }
        }

        // Automatically scroll down as the user navigates through the grid
        if (rowIndex >= maxRow) {
            rowIndex = maxRow - 1;
        }
        if (rowIndex < 0) {
            rowIndex = 0;
        }
    }

    // Manually concatenate the array elements into a string
    let finalWord = "";
    for (let i = 0; i < selectedWord.length; i++) {
        finalWord += selectedWord[i];
    }

    return finalWord;
}


// Function to check the user's guess against the target word
function checkGuess(guess, target) {
    let feedback = { correct: false, positions: [] };
    for (let i = 0; i < guess.length; i++) {
        let position = {};
        position.letter = guess[i];
        if (guess[i] === target[i]) {
            position.correct = true;
        } else if (target.indexOf(guess[i]) !== -1) {
            position.correct = false;
        } else {
            position.incorrect = true;
        }
        feedback.positions.push(position);
    }
    feedback.correct = (guess === target);
    return feedback;
}

// Function to display feedback to the user
function displayFeedback(feedback) {
    let message = "";
    for (let i = 0; i < feedback.positions.length; i++) {
        let pos = feedback.positions[i];
        if (pos.correct) {
            message += "[" + to_string(pos.letter) + "] ";
        } else if (pos.correct === false) {
            message += "(" + to_string(pos.letter) + ") ";
        } else {
            message += to_string(pos.letter) + " ";
        }
    }
    dialog.message("Feedback", message);
}

// Main game function
function playGame() {
    let targetWord = getRandomWord();
    let attempts = 5;
    let grid = createGrid(targetWord);

    while (attempts > 0) {
        let guess = getUserGuess(grid, attempts, targetWord);
        let feedback = checkGuess(guess, targetWord);
        displayFeedback(feedback);
        
        if (feedback.correct) {
            dialog.message("Result", "Congratulations! You guessed the word correctly.");
            return;
        }
        
        for (let i = 0; i < feedback.positions.length; i++) {
            if (feedback.positions[i].incorrect) {
                for (let j = 0; j < grid[i].length; j++) {
                    if (grid[i][j] === feedback.positions[i].letter) {
                        grid[i][j] = " ";
                    }
                }
            }
        }

        attempts--;
    }

    dialog.message("Result", "Game over! The correct word was: " + to_string(targetWord));
}

// Call the main game function
playGame();
