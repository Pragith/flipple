let dialog = require("dialog");
let math = require("math");

// Predefined list of words
let words = ["during", "return", "events", "little"];

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
function displayGrid(grid, centerIndex) {
    let gridDisplay = "";
    for (let i = 0; i < grid[0].length; i++) {
        for (let j = 0; j < grid.length; j++) {
            if (j === centerIndex) {
                gridDisplay += "[" + to_string(grid[j][i]) + "] ";
            } else {
                gridDisplay += " " + to_string(grid[j][i]) + "  ";
            }
        }
        gridDisplay += "\n";
    }
    return gridDisplay;
}

// Function to get the user's guess
function getUserGuess(grid, attempts) {
    let selectedWord = [];
    let centerIndex = math.floor(grid.length / 2);
    for (let i = 0; i < grid.length; i++) {
        let columnDisplay = displayGrid(grid, centerIndex);
        dialog.message("Select Letter", columnDisplay + "Remaining Turns: 🗝️ x " + to_string(attempts));
        let selectedLetter = dialog.input("Select Letter", "Enter the letter you choose from Column " + to_string(i + 1) + ":");
        selectedWord.push(selectedLetter);
    }
    return selectedWord.join("");
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
        let guess = getUserGuess(grid, attempts);
        dialog.message("Guess", guess);
        
        let feedback = checkGuess(guess, targetWord);
        displayFeedback(feedback);
        
        if (feedback.correct) {
            dialog.message("Result", "Congratulations! You guessed the word correctly.");
            return;
        }
        
        attempts--;
    }

    dialog.message("Result", "Game over! The correct word was: " + to_string(targetWord));
}

playGame();
