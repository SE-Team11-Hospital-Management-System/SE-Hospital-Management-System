// Small helpers shared by the pages.
export const today = () => {
  const d = new Date();
  return `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, '0')}-${String(d.getDate()).padStart(2, '0')}`;
};

// 08:00, 08:30, ... 19:30 — appointment start times (HMS-F-021)
export const slotTimes = Array.from({ length: 24 }, (_, i) => `${String(8 + Math.floor(i / 2)).padStart(2, '0')}:${i % 2 ? '30' : '00'}`);
// 08:00 ... 20:00 — working-hour boundaries
export const hourMarks = [...slotTimes, '20:00'];

export const weekdays = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];

// The server puts the next free slots in error.details as "2030-01-07 09:30, 2030-01-07 10:00"
export const alternatives = (details: string) => details.split(',').map((s) => s.trim()).filter(Boolean);

export const genders = [{ value: 'F', label: 'Female' }, { value: 'M', label: 'Male' }, { value: 'O', label: 'Other' }];
export const bloodGroups = ['A+', 'A-', 'B+', 'B-', 'AB+', 'AB-', 'O+', 'O-'];
