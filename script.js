const TOTAL_SEATS = 10;

const seats = Array.from({ length: TOTAL_SEATS }, (_, index) => ({
  seatNo: index + 1,
  booked: false,
  customerName: 'Available',
}));

const seatGrid = document.getElementById('seatGrid');
const reserveSeatSelect = document.getElementById('reserveSeat');
const cancelSeatSelect = document.getElementById('cancelSeat');
const reserveForm = document.getElementById('reserveForm');
const cancelForm = document.getElementById('cancelForm');
const customerNameInput = document.getElementById('customerName');
const messageBox = document.getElementById('message');
const availableSeatsDisplay = document.getElementById('availableSeats');

function setMessage(text, type = 'info') {
  messageBox.textContent = text;
  messageBox.className = `message ${type}`;
}

function renderOptions() {
  const createOptions = (selectedElement) => {
    const currentValue = Number(selectedElement.value) || 1;
    selectedElement.innerHTML = seats
      .map(
        (seat) => `
          <option value="${seat.seatNo}" ${seat.seatNo === currentValue ? 'selected' : ''}>
            Seat ${seat.seatNo}
          </option>
        `
      )
      .join('');
  };

  createOptions(reserveSeatSelect);
  createOptions(cancelSeatSelect);
}

function renderAvailableSeats() {
  const availableSeats = seats.filter((seat) => !seat.booked).map((seat) => seat.seatNo);
  availableSeatsDisplay.textContent = availableSeats.length ? availableSeats.join(', ') : 'None';
}

function renderSeatGrid() {
  seatGrid.innerHTML = seats.map((seat) => {
    const itemClass = `seat ${seat.booked ? 'booked' : ''}`;
    const label = seat.booked ? seat.customerName : 'Available';

    return `
      <button type="button" class="${itemClass}" data-seat-number="${seat.seatNo}" aria-label="Seat ${seat.seatNo}: ${label}">
        <span class="seat-number">${seat.seatNo}</span>
        <span class="seat-name">${label}</span>
      </button>
    `;
  }).join('');

  seatGrid.querySelectorAll('.seat').forEach((button) => {
    button.addEventListener('click', () => {
      const seatNo = Number(button.dataset.seatNumber);
      const seat = seats.find((item) => item.seatNo === seatNo);

      if (!seat) {
        return;
      }

      if (seat.booked) {
        setMessage(`Seat ${seatNo} is already allocated to ${seat.customerName}.`, 'error');
        return;
      }

      reserveSeatSelect.value = String(seatNo);
      customerNameInput.focus();
      setMessage(`Seat ${seatNo} is selected. Enter a customer name to allocate it.`, 'info');
    });
  });
}

function render() {
  renderOptions();
  renderAvailableSeats();
  renderSeatGrid();
}

function allocateSeat(seatNo, customerName) {
  const seat = seats.find((item) => item.seatNo === seatNo);

  if (!seat) {
    setMessage(`Seat ${seatNo} does not exist.`, 'error');
    return;
  }

  if (seat.booked) {
    setMessage(`Seat ${seatNo} is already allocated to ${seat.customerName}.`, 'error');
    return;
  }

  seat.booked = true;
  seat.customerName = customerName;
  render();
  setMessage(`Seat ${seatNo} booked successfully for ${customerName}.`, 'success');
}

function cancelReservation(seatNo) {
  const seat = seats.find((item) => item.seatNo === seatNo);

  if (!seat) {
    setMessage(`Seat ${seatNo} does not exist.`, 'error');
    return;
  }

  if (!seat.booked) {
    setMessage(`Seat ${seatNo} is already available.`, 'error');
    return;
  }

  seat.booked = false;
  seat.customerName = 'Available';
  render();
  setMessage(`Reservation for seat ${seatNo} cancelled.`, 'success');
}

reserveForm.addEventListener('submit', (event) => {
  event.preventDefault();
  const seatNo = Number(reserveSeatSelect.value);
  const customerName = customerNameInput.value.trim();

  if (!customerName) {
    setMessage('Please enter a customer name before allocating a seat.', 'error');
    customerNameInput.focus();
    return;
  }

  allocateSeat(seatNo, customerName);
  customerNameInput.value = '';
  reserveSeatSelect.value = String(seatNo);
});

cancelForm.addEventListener('submit', (event) => {
  event.preventDefault();
  const seatNo = Number(cancelSeatSelect.value);
  cancelReservation(seatNo);
});

render();
