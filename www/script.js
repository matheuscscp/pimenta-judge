
// keep the alphabetical order!
var problems = [];
problems["A"] = "#FF0000";
problems["B"] = "#00FF00";
problems["C"] = "#0000FF";
problems["D"] = "#FF6600";
problems["E"] = "#006600";
problems["F"] = "#003399";
problems["G"] = "#FFCC00";
problems["H"] = "#FFFFFF";
problems["I"] = "#000000";
problems["J"] = "#FFFF00";

function login() {
  team = document.getElementById("team");
  pass = document.getElementById("pass");
  if (team.value == "" || pass.value == "") {
    document.getElementById("response").innerHTML = "Invalid team/password!";
    return;
  }
  var xmlhttp;
  if (window.XMLHttpRequest)
    xmlhttp = new XMLHttpRequest();
  else
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      if (xmlhttp.responseText == "ok") {
        window.location = "/";
      }
      else {
        document.getElementById("response").innerHTML = xmlhttp.responseText;
      }
    }
  }
  xmlhttp.open("POST", "login", true);
  xmlhttp.setRequestHeader("Team", team.value);
  xmlhttp.setRequestHeader("Password", pass.value);
  xmlhttp.send();
  team.value = "";
  pass.value = "";
}

function init() {
  init_problems();
  init_data();
  submission();
}

function init_problems() {
  totalproblems = 0;
  for (p in problems) totalproblems++;
  opts = "<option></option>";
  for (i = 0; i < totalproblems; i++) {
    prob = String.fromCharCode(65+i);
    opts += ("<option value=\""+prob+"\">"+prob+"</option>");
  }
  document.getElementById("problem").innerHTML = opts;
}

function init_data() {
  show_data("teamname", "teamname", "Team: ", "", function() {});
  var upd_rem_time_counter = function() {
    var now = new Date().getTime()/1000;
    var dt = now-remaining_time_json.init_time;
    var tmp = Math.round(Math.max(0,remaining_time_json.remaining_time-dt));
    var ans = (tmp == 0 ? "The contest is not running." : "Remaining time: " + (tmp+"").toHHMMSS());
    document.getElementById("remaining-time").innerHTML = ans;
  };
  var upd_rem_time = function() {
    data("remaining-time", function(response) {
      window["remaining_time_json"] = {
        remaining_time: parseInt(response),
        init_time: new Date().getTime()/1000
      };
      upd_rem_time_counter();
    });
  };
  upd_rem_time();
  setInterval(upd_rem_time, 30000);
  setInterval(upd_rem_time_counter, 1000);
}

function data(key, cb) {
  var xmlhttp;
  if (window.XMLHttpRequest)
    xmlhttp = new XMLHttpRequest();
  else
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      cb(xmlhttp.responseText);
    }
  }
  xmlhttp.open("GET", "?"+key, true);
  xmlhttp.send();
}

function show_data(key, tagid, before, after, cb) {
  data(key, function(response) {
    document.getElementById(tagid).innerHTML = before+response+after;
    cb();
  });
}

function submission() {
  document.getElementById("content").innerHTML = document.getElementById("submission").innerHTML;
  document.getElementById("file").focus();
}
function scoreboard() {
  show_data("scoreboard", "content", "", "", function() {
    jQuery.get(jQuery("img.svg").attr("src"), function(data) {
      var $svg = jQuery(data).find("svg");
      jQuery("img.svg").each(function(){
        var $svgtmp = $svg.clone();
        var $img = jQuery(this);
        $svgtmp.attr("class", $img.attr("class"));
        $img.replaceWith($svgtmp);
      });
      for (p in problems) {
        $(".balloon."+p+" > path.balloonfill").css("fill", problems[p]);
      }
    });
  });
}
function clarifications() {
  show_data("clarifications", "content", document.getElementById("clarifications").innerHTML, "", function() {
    document.getElementById("problem").focus();
  });
}
function logout() {
  var xmlhttp;
  if (window.XMLHttpRequest)
    xmlhttp = new XMLHttpRequest();
  else
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      window.location = "/";
    }
  }
  xmlhttp.open("GET", "logout", true);
  xmlhttp.send();
}

function attempt() {
  document.getElementById("response").innerHTML = "Wait for the verdict.";
  file = document.getElementById("file");
  var xmlhttp;
  if (window.XMLHttpRequest)
    xmlhttp = new XMLHttpRequest();
  else
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      document.getElementById("response").innerHTML = xmlhttp.responseText;
    }
  }
  xmlhttp.open("POST", "attempt", true);
  xmlhttp.setRequestHeader("File-name", file.files[0].name);
  xmlhttp.setRequestHeader("File-size", file.files[0].size);
  xmlhttp.send(file.files[0]);
  file.value = "";
}

function question() {
  prob = document.getElementById("problem");
  if (prob.value == "") {
    document.getElementById("response").innerHTML = "Choose a problem!";
    return;
  }
  ques = document.getElementById("question");
  if (ques.value == "") {
    document.getElementById("response").innerHTML = "Write something!";
    return;
  }
  document.getElementById("response").innerHTML = "Question sent. Wait and refresh.";
  var xmlhttp;
  if (window.XMLHttpRequest)
    xmlhttp = new XMLHttpRequest();
  else
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      document.getElementById("response").innerHTML = xmlhttp.responseText;
    }
  }
  xmlhttp.open("POST", "question", true);
  xmlhttp.setRequestHeader("Problem", prob.value);
  xmlhttp.setRequestHeader("Question", ques.value);
  xmlhttp.send();
  prob.value = "";
  ques.value = "";
}

String.prototype.toHHMMSS = function () {
  var sec_num = parseInt(this, 10);
  var hours   = Math.floor(sec_num / 3600);
  var minutes = Math.floor((sec_num - (hours * 3600)) / 60);
  var seconds = sec_num - (hours * 3600) - (minutes * 60);
  if (hours   < 10) {hours   = "0"+hours;}
  if (minutes < 10) {minutes = "0"+minutes;}
  if (seconds < 10) {seconds = "0"+seconds;}
  return hours+':'+minutes+':'+seconds;
}
