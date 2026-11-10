const rows = document.getElementById('rows');
const message = document.getElementById('message');
let current = [];

async function loadStudents(url='/api/students') {
  try {
    const res = await fetch(url);
    current = await res.json();
    render(current);
  } catch (e) {
    message.textContent = 'Could not connect to the server. Make sure server.exe is running.';
    message.style.color = '#bd3434';
  }
}
function render(list) {
  rows.innerHTML = '';
  document.getElementById('empty').hidden = list.length !== 0;
  list.forEach(s => {
    const tr = document.createElement('tr');
    [s.roll,s.name,s.department,Number(s.cgpa).toFixed(2),s.attendance+'%'].forEach(value => {
      const td=document.createElement('td'); td.textContent=value; tr.appendChild(td);
    });
    const action=document.createElement('td');
    const btn=document.createElement('button'); btn.className='delete'; btn.textContent='Delete';
    btn.addEventListener('click',()=>deleteStudent(s.roll)); action.appendChild(btn); tr.appendChild(action);
    rows.appendChild(tr);
  });
  document.getElementById('total').textContent=list.length;
  const avg=list.length?list.reduce((a,s)=>a+Number(s.cgpa),0)/list.length:0;
  const att=list.length?list.reduce((a,s)=>a+Number(s.attendance),0)/list.length:0;
  document.getElementById('average').textContent=avg.toFixed(2);
  document.getElementById('attendance').textContent=att.toFixed(1)+'%';
}
async function deleteStudent(roll) {
  if(!confirm('Delete student with roll number '+roll+'?')) return;
  const body=new URLSearchParams({roll});
  const res=await fetch('/api/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  const data=await res.json(); message.textContent=data.message;
  message.style.color=data.ok?'#267447':'#bd3434'; loadStudents();
}
document.getElementById('addForm').addEventListener('submit',async e=>{
  e.preventDefault();
  const body=new URLSearchParams(new FormData(e.target));
  const res=await fetch('/api/add',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  const data=await res.json(); message.textContent=data.message;
  message.style.color=data.ok?'#267447':'#bd3434';
  if(data.ok){e.target.reset();loadStudents();}
});
let timer;
document.getElementById('search').addEventListener('input',e=>{
  clearTimeout(timer); const q=e.target.value.trim();
  timer=setTimeout(()=>loadStudents(q?'/api/search?q='+encodeURIComponent(q):'/api/students'),180);
});
document.getElementById('sort').addEventListener('change',e=>{
  loadStudents(e.target.value?'/api/sort?by='+e.target.value:'/api/students');
});
loadStudents();
