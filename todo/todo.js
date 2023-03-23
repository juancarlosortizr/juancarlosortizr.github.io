// Internal list that actually stores all tasks long-term
let todoList = [{name: "Example Task", done: false}];

function display() {
  // HTML object used purely to display; doesn't store anything long-term
  let todoListElement = document.getElementById("todo-list");
  // Avoid duplicate displays of the same task
  todoListElement.innerHTML = "";

  for (let i = 0; i < todoList.length; i++) {
    let task = todoList[i];
    let li = document.createElement("li");

    // Button to mark task as done
    let box = document.createElement("input");
    box.type = "checkbox";
    box.checked = task.done;
    box.addEventListener("change", () => {
      task.done = box.checked;
      display();
    });
    li.appendChild(box);

    // Text description of task
    let span = document.createElement("span");
    span.textContent = task.name + ' ';
    if (task.done) {span.style.textDecoration = "line-through";}
    li.appendChild(span);

    // Button to increase task's priority in list
    let upButton = document.createElement("button");
    upButton.textContent = "\u2191";
    upButton.addEventListener("click", () => {
      if (i > 0) {
        [todoList[i], todoList[i - 1]] = [todoList[i - 1], todoList[i]];
        display();
      }
    });
    li.appendChild(upButton);

    // Button to decrease task's priority in list
    let downButton = document.createElement("button");
    downButton.textContent = "\u2193";
    downButton.addEventListener("click", () => {
      if (i < todoList.length - 1) {
        [todoList[i], todoList[i + 1]] = [todoList[i + 1], todoList[i]];
        display();
      }
    });
    li.appendChild(downButton);

    // Button to completely kill a task
    let killButton = document.createElement("button");
    killButton.textContent = "\u274C";
    killButton.addEventListener("click", () => {
      todoList.splice(i, 1);
      display();
    })
    li.appendChild(killButton);


    todoListElement.appendChild(li);
  }
}

function addTask(name) {
  todoList.push({name, done: false});
  display();
}

document.getElementById("add-task").addEventListener("submit", (event) => {
  event.preventDefault();  // prevent whole page from rendering from scratch
  let newTaskInput = document.getElementById("new-task-name");
  addTask(newTaskInput.value);
  newTaskInput.value = "";  // reset input box to empty
});

display();