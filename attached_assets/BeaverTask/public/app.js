const API_ENDPOINT = '/api/tasks';

const state = {
  tasks: [],
  selectedDate: new Date(),
  currentYear: new Date().getFullYear(),
  currentMonth: new Date().getMonth(),
};

const monthNames = ["JAN.", "FEB.", "MAR.", "APR.", "MAY", "JUN.", "JUL.", "AUG.", "SEP.", "OCT.", "NOV.", "DEC."];
const monthNamesFull = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

const requestJson = async (url, options = {}) => {
  try {
    const response = await fetch(url, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...(options.headers || {}),
      },
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const text = await response.text();
    return text ? JSON.parse(text) : null;
  } catch (error) {
    console.error('Request failed:', error);
    throw error;
  }
};

const refreshTasks = async () => {
  try {
    const data = await requestJson(API_ENDPOINT);
    state.tasks = Array.isArray(data) ? data : [];
    return true;
  } catch (error) {
    console.error('Failed to refresh tasks:', error);
    state.tasks = [];
    return false;
  }
};

const addTask = async (taskData) => {
  try {
    const newTask = await requestJson(API_ENDPOINT, {
      method: 'POST',
      body: JSON.stringify(taskData),
    });
    
    if (newTask) {
      state.tasks.push(newTask);
      await refreshTasks();
      rerenderAll();
      return true;
    }
    return false;
  } catch (error) {
    console.error('Failed to add task:', error);
    return false;
  }
};

const updateTask = async (id, updates) => {
  try {
    const updatedTask = await requestJson(`${API_ENDPOINT}/${encodeURIComponent(id)}`, {
      method: 'PUT',
      body: JSON.stringify(updates),
    });
    
    if (updatedTask) {
      const index = state.tasks.findIndex((task) => task.id === id);
      if (index !== -1) {
        state.tasks[index] = updatedTask;
      }
      await refreshTasks();
      rerenderAll();
      return true;
    }
    return false;
  } catch (error) {
    console.error('Failed to update task:', error);
    return false;
  }
};

const deleteTask = async (id) => {
  try {
    await requestJson(`${API_ENDPOINT}/${encodeURIComponent(id)}`, {
      method: 'DELETE',
    });
    state.tasks = state.tasks.filter((task) => task.id !== id);
    await refreshTasks();
    rerenderAll();
    return true;
  } catch (error) {
    console.error('Failed to delete task:', error);
    return false;
  }
};

const getDateKey = (date) => {
  const year = date.getFullYear();
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const day = String(date.getDate()).padStart(2, '0');
  return `${year}-${month}-${day}`;
};

const ensureSelectedDate = () => {
  if (!(state.selectedDate instanceof Date)) {
    state.selectedDate = new Date();
  }
};

const parseDateInput = (value) => {
  if (!value || typeof value !== 'string') {
    return new Date();
  }

  const [year, month, day] = value.split('-').map(Number);
  if (!year || !month || !day) {
    return new Date();
  }

  return new Date(year, month - 1, day);
};

const renderCalendar = () => {
  const grid = document.getElementById('calendar-grid');
  const monthDisplay = document.getElementById('month-display');

  monthDisplay.textContent = monthNames[state.currentMonth];

  grid.innerHTML = '';

  const firstDay = new Date(state.currentYear, state.currentMonth, 1);
  const lastDay = new Date(state.currentYear, state.currentMonth + 1, 0);
  const startDay = firstDay.getDay();
  const totalDays = lastDay.getDate();

  const today = new Date();
  const todayKey = getDateKey(today);

  const taskMap = state.tasks.reduce((map, task) => {
    if (task.dueDate) {
      if (!map.has(task.dueDate)) {
        map.set(task.dueDate, []);
      }
      map.get(task.dueDate).push(task);
    }
    return map;
  }, new Map());

  for (let i = 0; i < startDay; i++) {
    const prevMonthDay = new Date(state.currentYear, state.currentMonth, -startDay + i + 1);
    const cell = createCalendarCell(prevMonthDay, true, taskMap);
    grid.appendChild(cell);
  }

  for (let day = 1; day <= totalDays; day++) {
    const date = new Date(state.currentYear, state.currentMonth, day);
    const cell = createCalendarCell(date, false, taskMap);
    grid.appendChild(cell);
  }

  const cellsCount = grid.children.length;
  const remainingCells = Math.ceil(cellsCount / 7) * 7 - cellsCount;
  for (let i = 1; i <= remainingCells; i++) {
    const nextMonthDay = new Date(state.currentYear, state.currentMonth + 1, i);
    const cell = createCalendarCell(nextMonthDay, true, taskMap);
    grid.appendChild(cell);
  }
};

const createCalendarCell = (date, inactive, taskMap) => {
  const cell = document.createElement('div');
  cell.className = 'calendar-cell';

  const dateKey = getDateKey(date);
  const today = new Date();
  const todayKey = getDateKey(today);

  if (inactive) {
    cell.classList.add('inactive');
  }

  if (dateKey === todayKey) {
    cell.classList.add('today');
  }

  if (state.selectedDate && getDateKey(state.selectedDate) === dateKey) {
    cell.classList.add('selected');
  }

  if (taskMap.has(dateKey)) {
    cell.classList.add('has-events');
  }

  const dateSpan = document.createElement('span');
  dateSpan.className = 'cell-date';
  dateSpan.textContent = date.getDate();
  cell.appendChild(dateSpan);

  cell.addEventListener('click', () => {
    state.selectedDate = date;
    renderCalendar();
    renderDetails();
  });

  return cell;
};

const renderDetails = () => {
  const detailsHeader = document.getElementById('details-header');
  const detailsContent = document.getElementById('details-content');

  ensureSelectedDate();

  const dateKey = getDateKey(state.selectedDate);
  const day = state.selectedDate.getDate();
  const month = monthNamesFull[state.selectedDate.getMonth()].substring(0, 3);

  detailsHeader.textContent = `${day} ${month}.`;

  const tasksForDate = state.tasks.filter(task => task.dueDate === dateKey);

  if (tasksForDate.length === 0) {
    detailsContent.innerHTML = `
      <div class="empty-state">
        <div class="empty-icon">☺</div>
        <div class="empty-text">Aucun événement pour cette journée. Cliquez sur le bouton + pour en ajouter un.</div>
      </div>
    `;
  } else {
    const taskListHTML = tasksForDate.map(task => {
      const metaParts = [];
      
      if (task.allDay) {
        metaParts.push('Toute la journée');
      } else if (task.startTime) {
        const timeRange = task.endTime ? `${task.startTime} - ${task.endTime}` : task.startTime;
        metaParts.push(`🕐 ${timeRange}`);
      }
      
      if (task.location) {
        metaParts.push(`📍 ${task.location}`);
      }
      
      if (task.reminderMinutes > 0) {
        const reminderText = task.reminderMinutes >= 1440 
          ? `${Math.floor(task.reminderMinutes / 1440)} jour(s) avant`
          : task.reminderMinutes >= 60 
          ? `${Math.floor(task.reminderMinutes / 60)} heure(s) avant`
          : `${task.reminderMinutes} min avant`;
        metaParts.push(`🔔 ${reminderText}`);
      }
      
      if (task.tag) metaParts.push(task.tag);
      if (task.description) metaParts.push(task.description);
      
      const metaHTML = metaParts.length > 0 ? `<div class="task-meta">${metaParts.join(' • ')}</div>` : '';
      const colorIndicator = task.color ? `<div class="task-color-dot" style="background: ${task.color};"></div>` : '';
      
      return `
        <div class="task-item">
          <div class="task-checkbox ${task.completed ? 'completed' : ''}" data-id="${task.id}"></div>
          <div class="task-info">
            <div class="task-title">
              ${colorIndicator}
              <span>${task.title}</span>
            </div>
            ${metaHTML}
          </div>
          <div class="task-actions">
            <button class="task-action-btn delete" data-action="delete" data-id="${task.id}">×</button>
          </div>
        </div>
      `;
    }).join('');

    detailsContent.innerHTML = `<div class="task-list">${taskListHTML}</div>`;

    detailsContent.querySelectorAll('.task-checkbox').forEach((checkbox) => {
      checkbox.addEventListener('click', async (event) => {
        const id = event.currentTarget.dataset.id;
        const task = state.tasks.find((t) => t.id === id);
        if (task) {
          await updateTask(id, { completed: !task.completed });
        }
      });
    });

    detailsContent.querySelectorAll('.task-action-btn.delete').forEach((btn) => {
      btn.addEventListener('click', async (event) => {
        event.stopPropagation();
        const id = event.currentTarget.dataset.id;
        if (confirm('Supprimer cette tâche ?')) {
          await deleteTask(id);
        }
      });
    });
  }
};

const changeMonth = (offset) => {
  state.currentMonth += offset;
  if (state.currentMonth < 0) {
    state.currentMonth = 11;
    state.currentYear -= 1;
  } else if (state.currentMonth > 11) {
    state.currentMonth = 0;
    state.currentYear += 1;
  }
  renderCalendar();
  renderDetails();
};

const updateCurrentDayBadge = () => {
  const today = new Date();
  const badge = document.getElementById('current-day-badge');
  badge.textContent = today.getDate();
};

const detailsAddBtn = document.getElementById('details-add-btn');
const taskModal = document.getElementById('task-modal');
const taskForm = document.getElementById('task-form');
const taskTitleInput = document.getElementById('task-title');
const taskTagInput = document.getElementById('task-tag');
const taskDescriptionInput = document.getElementById('task-description');
const taskAllDayInput = document.getElementById('task-allday');
const taskStartTimeInput = document.getElementById('task-start-time');
const taskEndTimeInput = document.getElementById('task-end-time');
const taskLocationInput = document.getElementById('task-location');
const taskReminderInput = document.getElementById('task-reminder');
const taskCancelBtn = document.getElementById('task-cancel');
const taskModalDescription = document.getElementById('task-modal-description');
const timeFields = document.getElementById('time-fields');

const openTaskModal = () => {
  if (!taskModal) {
    return;
  }

  ensureSelectedDate();
  const day = state.selectedDate.getDate();
  const month = monthNamesFull[state.selectedDate.getMonth()];
  const year = state.selectedDate.getFullYear();
  
  taskModalDescription.textContent = `Ajouter un événement pour le ${day} ${month} ${year}.`;
  
  if (taskAllDayInput) {
    taskAllDayInput.checked = false;
    timeFields.style.display = 'grid';
  }
  
  taskModal.removeAttribute('hidden');
  requestAnimationFrame(() => {
    taskTitleInput?.focus();
  });
};

taskAllDayInput?.addEventListener('change', (event) => {
  if (timeFields) {
    timeFields.style.display = event.target.checked ? 'none' : 'grid';
  }
});

const closeTaskModal = () => {
  if (!taskModal) {
    return;
  }

  taskModal.setAttribute('hidden', '');
  taskForm?.reset();
};

document.getElementById('prev-month').addEventListener('click', () => changeMonth(-1));
document.getElementById('next-month').addEventListener('click', () => changeMonth(1));

detailsAddBtn?.addEventListener('click', openTaskModal);

taskCancelBtn?.addEventListener('click', closeTaskModal);

taskModal?.addEventListener('click', (event) => {
  if (event.target === taskModal) {
    closeTaskModal();
  }
});

taskForm?.addEventListener('submit', async (event) => {
  event.preventDefault();

  const title = taskTitleInput?.value.trim();
  if (!title) {
    taskTitleInput?.focus();
    return;
  }

  ensureSelectedDate();
  const descriptionValue = taskDescriptionInput?.value.trim() ?? '';
  const tagValue = taskTagInput?.value.trim() ?? '';
  const allDayValue = taskAllDayInput?.checked ?? false;
  const startTimeValue = taskStartTimeInput?.value ?? '';
  const endTimeValue = taskEndTimeInput?.value ?? '';
  const locationValue = taskLocationInput?.value.trim() ?? '';
  const reminderValue = parseInt(taskReminderInput?.value ?? '0', 10);
  
  const colorInputs = document.querySelectorAll('input[name="color"]');
  let colorValue = '#f89422';
  colorInputs.forEach(input => {
    if (input.checked) {
      colorValue = input.value;
    }
  });

  const success = await addTask({
    title,
    dueDate: getDateKey(state.selectedDate),
    description: descriptionValue,
    tag: tagValue,
    startTime: allDayValue ? '' : startTimeValue,
    endTime: allDayValue ? '' : endTimeValue,
    allDay: allDayValue,
    location: locationValue,
    reminderMinutes: reminderValue,
    color: colorValue,
    completed: false,
  });

  if (success) {
    closeTaskModal();
  }
});

document.addEventListener('keydown', (event) => {
  if (event.key === 'Escape') {
    if (taskModal && !taskModal.hasAttribute('hidden')) {
      closeTaskModal();
    } else if (dateJumpModal && !dateJumpModal.hasAttribute('hidden')) {
      closeDateJumpModal();
    }
  }
});

const dateJumpModal = document.getElementById('date-jump-modal');
const dateJumpForm = document.getElementById('date-jump-form');
const jumpDateInput = document.getElementById('jump-date');
const dateJumpCancelBtn = document.getElementById('date-jump-cancel');

const openDateJumpModal = () => {
  if (!dateJumpModal) {
    return;
  }

  ensureSelectedDate();
  jumpDateInput.value = getDateKey(state.selectedDate);
  
  dateJumpModal.removeAttribute('hidden');
  requestAnimationFrame(() => {
    jumpDateInput?.focus();
  });
};

const closeDateJumpModal = () => {
  if (!dateJumpModal) {
    return;
  }

  dateJumpModal.setAttribute('hidden', '');
  dateJumpForm?.reset();
};

document.getElementById('search-btn').addEventListener('click', openDateJumpModal);

dateJumpCancelBtn?.addEventListener('click', closeDateJumpModal);

dateJumpModal?.addEventListener('click', (event) => {
  if (event.target === dateJumpModal) {
    closeDateJumpModal();
  }
});

dateJumpForm?.addEventListener('submit', (event) => {
  event.preventDefault();

  const dateValue = jumpDateInput?.value;
  if (!dateValue) {
    return;
  }

  const targetDate = parseDateInput(dateValue);
  state.currentYear = targetDate.getFullYear();
  state.currentMonth = targetDate.getMonth();
  state.selectedDate = targetDate;
  
  renderCalendar();
  renderDetails();
  closeDateJumpModal();
});

const showCalendarView = () => {
  document.querySelector('.calendar-section').style.display = 'flex';
  document.querySelector('.details-panel').style.display = 'flex';
  document.getElementById('tasklist-section').classList.remove('active');
  document.getElementById('calendar-icon').classList.add('active');
  document.getElementById('tasklist-icon').classList.remove('active');
};

const showTaskListView = () => {
  document.querySelector('.calendar-section').style.display = 'none';
  document.querySelector('.details-panel').style.display = 'none';
  document.getElementById('tasklist-section').classList.add('active');
  document.getElementById('calendar-icon').classList.remove('active');
  document.getElementById('tasklist-icon').classList.add('active');
  renderTaskList();
};

const renderTaskList = () => {
  const tasklistContent = document.getElementById('tasklist-content');

  if (state.tasks.length === 0) {
    tasklistContent.innerHTML = `
      <div class="tasklist-empty">
        <div class="empty-icon">📋</div>
        <div class="empty-text">No tasks yet. Add your first task!</div>
      </div>
    `;
    return;
  }

  const today = new Date();
  today.setHours(0, 0, 0, 0);
  
  const incomplete = state.tasks.filter(t => !t.completed);
  const completed = state.tasks.filter(t => t.completed);
  
  const upcoming = incomplete.filter(t => {
    if (!t.dueDate) return false;
    const taskDate = new Date(t.dueDate);
    return taskDate >= today;
  }).sort((a, b) => new Date(a.dueDate) - new Date(b.dueDate));

  const overdue = incomplete.filter(t => {
    if (!t.dueDate) return false;
    const taskDate = new Date(t.dueDate);
    return taskDate < today;
  }).sort((a, b) => new Date(a.dueDate) - new Date(b.dueDate));

  const noDueDate = incomplete.filter(t => !t.dueDate);
  
  let html = '';

  if (overdue.length > 0) {
    html += `
      <div class="tasklist-group">
        <div class="tasklist-group-title">Overdue (${overdue.length})</div>
        <div class="task-list">
          ${overdue.map(task => renderTaskListItem(task)).join('')}
        </div>
      </div>
    `;
  }

  if (upcoming.length > 0) {
    html += `
      <div class="tasklist-group">
        <div class="tasklist-group-title">Upcoming (${upcoming.length})</div>
        <div class="task-list">
          ${upcoming.map(task => renderTaskListItem(task)).join('')}
        </div>
      </div>
    `;
  }

  if (noDueDate.length > 0) {
    html += `
      <div class="tasklist-group">
        <div class="tasklist-group-title">No Due Date (${noDueDate.length})</div>
        <div class="task-list">
          ${noDueDate.map(task => renderTaskListItem(task)).join('')}
        </div>
      </div>
    `;
  }

  if (completed.length > 0) {
    html += `
      <div class="tasklist-group">
        <div class="tasklist-group-title">Completed (${completed.length})</div>
        <div class="task-list">
          ${completed.map(task => renderTaskListItem(task)).join('')}
        </div>
      </div>
    `;
  }

  tasklistContent.innerHTML = html;

  tasklistContent.querySelectorAll('.task-checkbox').forEach((checkbox) => {
    checkbox.addEventListener('click', async (event) => {
      const id = event.currentTarget.dataset.id;
      const task = state.tasks.find((t) => t.id === id);
      if (task) {
        await updateTask(id, { completed: !task.completed });
      }
    });
  });

  tasklistContent.querySelectorAll('.task-action-btn.delete').forEach((btn) => {
    btn.addEventListener('click', async (event) => {
      const id = event.currentTarget.dataset.id;
      if (confirm('Delete this task?')) {
        await deleteTask(id);
      }
    });
  });
};

function rerenderAll() {
  renderCalendar();
  renderDetails();
  renderTaskList();
}

const renderTaskListItem = (task) => {
  const formattedDate = task.dueDate
    ? new Date(task.dueDate).toLocaleDateString('en-US', { month: 'short', day: 'numeric', year: 'numeric' })
    : 'No due date';
  
  const tagDisplay = task.tag ? ` • ${task.tag}` : '';
  
  return `
    <div class="task-item">
      <div class="task-checkbox ${task.completed ? 'completed' : ''}" data-id="${task.id}"></div>
      <div class="task-info">
        <div class="task-title">${task.title}</div>
        <div class="task-meta">${formattedDate}${tagDisplay}</div>
      </div>
      <div class="task-actions">
        <button class="task-action-btn delete" data-id="${task.id}">🗑️</button>
      </div>
    </div>
  `;
};

document.getElementById('calendar-icon').addEventListener('click', showCalendarView);
document.getElementById('tasklist-icon').addEventListener('click', showTaskListView);

const init = async () => {
  await refreshTasks();
  state.selectedDate = new Date();
  updateCurrentDayBadge();
  rerenderAll();
};

init().catch((error) => {
  console.error('Failed to initialize BeaverTask:', error);
});
