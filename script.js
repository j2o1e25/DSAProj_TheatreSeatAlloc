const seatGrid = document.getElementById('seatGrid');
const reserveSeatSelect = document.getElementById('reserveSeat');
const cancelSeatSelect = document.getElementById('cancelSeat');
const reserveForm = document.getElementById('reserveForm');
const cancelForm = document.getElementById('cancelForm');
const customerNameInput = document.getElementById('customerName');
const messageBox = document.getElementById('message');
const messageText = document.getElementById('messageText');
const reserveButton = document.getElementById('reserveButton');
const cancelButton = document.getElementById('cancelButton');
const selectedSeats = document.getElementById('seatCount');
const availableCount = document.getElementById('availableCount');
const bookedCount = document.getElementById('bookedCount');
const totalCount = document.getElementById('totalCount');

let seats = [];
let selectedSeatNo = null;

function setMessage(text, type = 'info') {
  messageText.textContent = text;
  messageBox.className = `message ${type}`;
}

function createOption(label, value, selected = false) {
  const option = document.createElement('option');
  option.value = value;
  option.textContent = label;
  option.selected = selected;
  return option;
}

function updateSelects() {
  const available = seats.filter((seat) => !seat.booked);
  const booked = seats.filter((seat) => seat.booked);
  const availableSelection = available.some((seat) => seat.seatNo === selectedSeatNo)
    ? String(selectedSeatNo)
    : available.length ? String(available[0].seatNo) : '';
  const cancelSelection = booked.some((seat) => seat.seatNo === Number(cancelSeatSelect.value))
    ? cancelSeatSelect.value
    : booked.length ? String(booked[0].seatNo) : '';

  reserveSeatSelect.replaceChildren();
  if (available.length) {
    available.forEach((seat) => {
      reserveSeatSelect.append(createOption(`Seat ${seat.seatNo}`, String(seat.seatNo), String(seat.seatNo) === availableSelection));
    });
  } else {
    reserveSeatSelect.append(createOption('No seats available', '', true));
  }

  cancelSeatSelect.replaceChildren();
  if (booked.length) {
    booked.forEach((seat) => {
      cancelSeatSelect.append(createOption(`Seat ${seat.seatNo} · ${seat.customerName}`, String(seat.seatNo), String(seat.seatNo) === cancelSelection));
    });
  } else {
    cancelSeatSelect.append(createOption('No reservations yet', '', true));
  }

  reserveButton.disabled = available.length === 0;
  cancelButton.disabled = booked.length === 0;
  availableCount.textContent = String(available.length);
  bookedCount.textContent = String(booked.length);
  totalCount.textContent = String(seats.length);
  selectedSeats.textContent = `${seats.length} ${seats.length === 1 ? 'seat' : 'seats'}`;
}

function renderSeatGrid() {
  seatGrid.replaceChildren();
  seats.forEach((seat) => {
    const button = document.createElement('button');
    const isSelected = !seat.booked && seat.seatNo === Number(reserveSeatSelect.value);
    button.type = 'button';
    button.className = `seat${seat.booked ? ' booked' : ''}${isSelected ? ' selected' : ''}`;
    button.dataset.seatNumber = String(seat.seatNo);
    button.setAttribute('aria-label', seat.booked
      ? `Seat ${seat.seatNo}, reserved for ${seat.customerName}. Select to cancel this reservation.`
      : `Seat ${seat.seatNo}, available. Select to reserve.`);

    const number = document.createElement('span');
    number.className = 'seat-number';
    number.textContent = String(seat.seatNo);
    const label = document.createElement('span');
    label.className = 'seat-label';
    label.textContent = seat.booked ? seat.customerName : 'Available';
    button.append(number, label);

    button.addEventListener('click', () => {
      if (seat.booked) {
        cancelSeatSelect.value = String(seat.seatNo);
        setMessage(`Seat ${seat.seatNo} is reserved for ${seat.customerName}. It’s selected if you need to cancel it.`, 'info');
        return;
      }

      selectedSeatNo = seat.seatNo;
      reserveSeatSelect.value = String(seat.seatNo);
      renderSeatGrid();
      customerNameInput.focus();
      setMessage(`Seat ${seat.seatNo} selected. Add a guest name to reserve it.`, 'info');
    });
    seatGrid.append(button);
  });
  seatGrid.setAttribute('aria-busy', 'false');
}

function render() {
  updateSelects();
  renderSeatGrid();
}

async function readResponse(response) {
  const result = await response.json();
  if (!response.ok) {
    throw new Error(result.message || `The box office returned an error (${response.status}).`);
  }
  return result;
}

async function refreshSeats() {
  seatGrid.setAttribute('aria-busy', 'true');
  try {
    const response = await fetch('/api/seats');
    seats = await readResponse(response);
    if (!Array.isArray(seats)) {
      throw new Error('The box office returned an unexpected seat list.');
    }
    render();
    setMessage('The box office is connected. Select an available seat to make a reservation.', 'info');
  } catch (error) {
    seatGrid.setAttribute('aria-busy', 'false');
    setMessage(`${error.message} Start the C server with “theatre-web.exe --server” and reload this page.`, 'error');
    reserveButton.disabled = true;
    cancelButton.disabled = true;
  }
}

reserveSeatSelect.addEventListener('change', () => {
  selectedSeatNo = Number(reserveSeatSelect.value) || null;
  renderSeatGrid();
});

reserveForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  const seatNo = Number(reserveSeatSelect.value);
  const customerName = customerNameInput.value.trim();
  if (!seatNo || !customerName) {
    setMessage('Choose an available seat and enter the guest’s name.', 'error');
    return;
  }

  reserveButton.disabled = true;
  try {
    const response = await fetch('/api/reservations', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ seatNo, customerName }),
    });
    const result = await readResponse(response);
    customerNameInput.value = '';
    selectedSeatNo = null;
    await refreshSeats();
    setMessage(result.message, 'success');
  } catch (error) {
    setMessage(error.message, 'error');
    reserveButton.disabled = false;
  }
});

cancelForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  const seatNo = Number(cancelSeatSelect.value);
  if (!seatNo) {
    setMessage('Choose a reserved seat to cancel.', 'error');
    return;
  }

  cancelButton.disabled = true;
  try {
    const response = await fetch(`/api/reservations/${seatNo}`, { method: 'DELETE' });
    const result = await readResponse(response);
    await refreshSeats();
    setMessage(result.message, 'success');
  } catch (error) {
    setMessage(error.message, 'error');
    cancelButton.disabled = false;
  }
});

refreshSeats();
