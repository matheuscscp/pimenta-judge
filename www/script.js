
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
  if (window.XMLHttpRequest)
    xmlhttp = new XMLHttpRequest();
  else
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      if (xmlhttp.responseText[0] == 'I') {
        document.getElementById("response").innerHTML = xmlhttp.responseText;
      }
      else {
        window.location = "/";
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
  data("teamname", "teamname", "Team: ", "", function(){});
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

function data(key, tagid, before, after, cb) {
  if (window.XMLHttpRequest)
    xmlhttp = new XMLHttpRequest();
  else
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      document.getElementById(tagid).innerHTML = before+xmlhttp.responseText+after;
      cb();
    }
  }
  xmlhttp.open("GET", "?"+key, true);
  xmlhttp.send();
}

function submission() {
  document.getElementById("content").innerHTML = document.getElementById("submission").innerHTML;
  document.getElementById("file").focus();
}
function scoreboard() {
  data("scoreboard", "content", "", "", function() {
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
  data("clarifications", "content", document.getElementById("clarifications").innerHTML, "", function() {
    document.getElementById("problem").focus();
  });
}

function attempt() {
  document.getElementById("response").innerHTML = "Wait for the verdict.";
  file = document.getElementById("file");
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
