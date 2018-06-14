const char connect_html[] PROGMEM = R"(<head>
<title>OpenSprinkler</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
</head>
<body>
<caption><b>OpenSprinkler Config</caption><br><br>
<table cellspacing=4 id='rd'>
<tr><td>(Scanning...)</td></tr>
</table>
<table cellspacing=16>
<tr><td><input type='text' name='ssid' id='ssid'></td><td>(SSID)</td></tr>
<tr><td><input type='password' name='pass' id='pass'></td><td>(Password)</td></tr>
<tr><td colspan=2><p id='msg'></p></td></tr>
<tr><td><button type='button' id='butt' onclick='sf();' style='height:36px;width:180px'>Submit</button></td><td></td></tr>
</table>
<script>
function id(s) {return document.getElementById(s);}
function sel(i) {id('ssid').value=id('rd'+i).value;}
var tci;
function tryConnect() {
var xhr=new XMLHttpRequest();
xhr.onreadystatechange=function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText);
if(jd.ip==0) return;
var ip=''+(jd.ip%256)+'.'+((jd.ip/256>>0)%256)+'.'+(((jd.ip/256>>0)/256>>0)%256)+'.'+(((jd.ip/256>>0)/256>>0)/256>>0);
id('msg').innerHTML='<b><font color=green>Connected! Device IP: '+ip+'</font></b><br>Device is rebooting. Switch back to<br>the above WiFi network, and then<br>click the button below to redirect.'
id('butt').innerHTML='Go to '+ip;id('butt').disabled=false;
id('butt').onclick=function rd(){window.open('http://'+ip);}
clearInterval(tci);
}
}    
xhr.open('POST', 'jt', true); xhr.send();    
}
function sf() {
id('msg').innerHTML='';
var xhr=new XMLHttpRequest();
xhr.onreadystatechange=function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText);
if(jd.result==1) { tci=setInterval(tryConnect, 10000); return; }
id('msg').innerHTML='<b><font color=red>Error code: '+jd.result+', item: '+jd.item+'</font></b>';
id('butt').innerHTML='Submit';
id('butt').disabled=false;id('ssid').disabled=false;id('pass').disabled=false;id('auth').disabled=false;
}
}
var comm='cc?ssid='+encodeURIComponent(id('ssid').value)+'&pass='+encodeURIComponent(id('pass').value);
xhr.open('POST', comm, true); xhr.send();
id('butt').innerHTML='Connecting...';
id('butt').disabled=true;id('ssid').disabled=true;id('pass').disabled=true;
}

function loadSSIDs() {
var xhr=new XMLHttpRequest();
xhr.onreadystatechange=function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText), i;
id('rd').deleteRow(0);
for(i=0;i<jd.ssids.length;i++) {
var cell=id('rd').insertRow(-1).insertCell(0);
cell.innerHTML ="<tr><td><input type='radio' name='ssids' id='rd" + i + "' value='" + jd.ssids[i] + "' onclick=sel(" + i + ")>" + jd.ssids[i] + "</td></tr>";
}
}
};
xhr.open('GET','js',true); xhr.send();
}
setTimeout(loadSSIDs, 1000);
</script>
</body>)";

const char update_html[] PROGMEM = R"(<head>
<title>OpenSprinkler</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' type='text/css'>
<script src='http://code.jquery.com/jquery-1.9.1.min.js' type='text/javascript'></script>
<script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js' type='text/javascript'></script>
</head>
<body>
<div data-role='page' id='page_update'>
<div data-role='header'><h3>OpenSprinkler Firmware Update</h3></div>
<div data-role='content'>
<form method='POST' action='/update' id='fm' enctype='multipart/form-data'>
<table cellspacing=4>
<tr><td><input type='file' name='file' accept='.bin' id='file'></td></tr>
<tr><td><b>Device password: </b><input type='password' name='pw' size=36 maxlength=36 id='pw'></td></tr>
<tr><td><label id='msg'></label></td></tr>
</table>
<div data-role='controlgroup' data-type='horizontal'>
<a href='#' data-role='button' data-inline='true' data-theme='b' id='btn_submit'>Submit</a>
</div>
</form>
</div>
<div data-role='footer' data-theme='c'>
<p style='font-weight:normal;'>&copy; OpenSprinkler (<a href='http://www.opensprinkler.com' target='_blank' style='text-decoration:none'>www.opensprinkler.com</a>)</p>
</div>  
</div>
<script>
function id(s) {return document.getElementById(s);}
function clear_msg() {id('msg').innerHTML='';}
function show_msg(s,t,c) {
id('msg').innerHTML=s.fontcolor(c);
if(t>0) setTimeout(clear_msg, t);
}  
$('#btn_submit').click(function(e){
var files= id('file').files;
if(files.length==0) {show_msg('Please select a file.',2000,'red'); return;}
if(id('pw').value=='') {
if(!confirm('You did not input a device key. Are you sure?')) return;
}else{
beforeSubmit();
}
show_msg('Uploading. Please wait...',0,'green');
var fd = new FormData();
var file = files[0];
fd.append('file', file, file.name);
fd.append('pw', id('pw').value);
var xhr = new XMLHttpRequest();
xhr.onreadystatechange = function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText);
if(jd.result==1) {
show_msg('Update is successful. Rebooting. Please wait...',10000,'green');
id('pw').value='';
} else if (jd.result==2) {
show_msg('Check device key and try again.', 10000, 'red');
} else {
show_msg('Update failed.',10000,'red');
}
}
};
xhr.open('POST', 'update', true);
xhr.send(fd);
});
</script>
<script src=https://ui.opensprinkler.com/js/hasher.js></script>
</body>)";


