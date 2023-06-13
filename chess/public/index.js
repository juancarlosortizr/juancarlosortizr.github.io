function playMove() {
  const userMove = document.getElementById('userMove').value;

  fetch(`/api/chess?userMove=${userMove}`)
    .then(response => response.json())
    .then(data => {
      const outputElement = document.getElementById('output');
      outputElement.textContent = data.board;
      console.log(data.board);
    })
    .catch(error => {
      console.error('Error:', error);
    });
}
