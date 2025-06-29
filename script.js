window.addEventListener('DOMContentLoaded', () => {
  const message = document.getElementById('message');
  const countDisplay = document.getElementById('count');
  const button = document.getElementById('increment-btn');

  message.textContent = "JS loaded and working!";
  
  let count = 0;

  button.addEventListener('click', () => {
    count++;
    countDisplay.textContent = count;
  });
});
