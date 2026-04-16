let dialog = require("dialog");
let math = require("math");

function getRandomNumber(min, max) {
    return math.floor(math.random() * (max - min + 1)) + min;
}

function playGame() {
    let targetNumber = getRandomNumber(1, 100);
    let attempts = 5;
    let guess;
    let message;

    while (attempts > 0) {
        let currentNumber = getRandomNumber(1, 100);
        let userChoice = dialog.custom({
            header: "Guess the Number",
            text: "Target Number: ?\nCurrent Number: " + to_string(currentNumber),
            button_left: "Lower",
            button_right: "Higher",
            button_center: "Equal"
        });

        if (userChoice === "Equal") {
            if (currentNumber === targetNumber) {
                print("Congratulations! You guessed the number: " + to_string(targetNumber));
                return;
            } else {
                message = "Incorrect! The number is not " + to_string(currentNumber);
            }
        } else if (userChoice === "Higher") {
            if (currentNumber > targetNumber) {
                message = "Correct! " + to_string(currentNumber) + " is higher.";
            } else {
                message = "Incorrect! " + to_string(currentNumber) + " is not higher.";
            }
        } else if (userChoice === "Lower") {
            if (currentNumber < targetNumber) {
                message = "Correct! " + to_string(currentNumber) + " is lower.";
            } else {
                message = "Incorrect! " + to_string(currentNumber) + " is not lower.";
            }
        }

        print(message);
        attempts--;

        if (attempts === 0) {
            print("Game over! The target number was: " + to_string(targetNumber));
        }
    }
}

playGame();
