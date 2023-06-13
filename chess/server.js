const express = require('express');
const app = express();
const path = require('path');
const chessModule = require('./build/Release/chess.node');

// Serve static files from the 'public' directory                                                                                                                         
app.use(express.static('public'));

// Define an API endpoint to handle the request                                                                                                                           
app.get('/api/chess', (req, res) => {
  const userMove = req.query.userMove;

  // Call your native module function and get the result
  const board = chessModule.playMove(userMove);

  // Return the result as a JSON response                                                                                                                                 
  res.setHeader('Content-Type', 'application/json');
  res.json({board});
});

// Start the server and listen on a specific port                                                                                                                          
const port = 3000;
app.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});
