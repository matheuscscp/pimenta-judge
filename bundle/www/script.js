function fix_balloons() {
  jQuery.get(jQuery("img.svg").attr("src"), function(data) {
    var $svg = jQuery(data).find("svg");
    jQuery("img.svg").each(function(){
      var $svgtmp = $svg.clone();
      var $img = jQuery(this);
      $svgtmp.attr("class", $img.attr("class"));
      $img.replaceWith($svgtmp);
    });
    for (var i = 0; i < svstatus.problems.length; i++) {
      $(".balloon."+svstatus.problems[i].name+" > path.balloonfill").css("fill",svstatus.problems[i].color);
    }
  });
}

function login() {
  user = document.getElementById("user");
  pass = document.getElementById("pass");
  if (user.value == "" || pass.value == "") {
    document.getElementById("response").innerHTML = "Invalid username/password!";
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
  xmlhttp.send(JSON.stringify({username: user.value, password: pass.value}));
  user.value = "";
  pass.value = "";
}

function lang_table(langs) {
  var ans =
    "<option></option>"
  ;
  for (var i = 0; i < langs.length; i++) {
    var flags = (langs[i].flags != "" ? " ("+langs[i].flags+")" : "");
    ans += "<option value=\""+langs[i].ext+"\">"+langs[i].name+flags+"</option>";
  }
  return ans;
}

function limits_table(langs,probs) {
  var ans =
    "<table class=\"data\">"+
      "<tr>"+
        "<th class=\"green\">Language</th>"+
        "<th class=\"blue\">Problem</th>"
  ;
  for (var i = 0; i < probs.length; i++) {
    ans +=
        "<th class=\"blue\">"+probs[i].name+"</th>"
    ;
  }
  ans +=
      "</tr>"
  ;
  for (var i = 0; i < langs.length; i++) {
    var zeb = (i%2 ? "" : " class=\"gray\"");
    ans +=
      "<tr>"+
        "<th class=\"green\" rowspan=\"2\">"+langs[i].name+"</th>"+
        "<th"+zeb+">Time limit</th>"
    ;
    for (var j = 0; j < probs.length; j++) {
      var lim = probs[j]["timelimit"];
      if (
        probs[j][langs[i].ext] != undefined &&
        probs[j][langs[i].ext]["timelimit"] != undefined
      ) {
        lim = probs[j][langs[i].ext].timelimit;
      }
      ans +=
        "<td"+zeb+">"+lim+" s</td>"
      ;
    }
    ans +=
      "</tr>"+
      "<tr>"+
        "<th"+zeb+">Memory limit</th>"
    ;
    for (var j = 0; j < probs.length; j++) {
      var lim = probs[j]["memlimit"];
      if (
        probs[j][langs[i].ext] != undefined &&
        probs[j][langs[i].ext]["memlimit"] != undefined) {
        lim = probs[j][langs[i].ext].memlimit;
      }
      ans +=
        "<td"+zeb+">"+lim+" kB</td>"
      ;
    }
    ans +=
      "</tr>"
    ;
  }
  return ans+"</table>";
}

function init_problems() {
  opts = "<option></option>";
  for (i = 0; i < svstatus.problems.length; i++) {
    prob = svstatus.problems[i].name;
    opts += ("<option value=\""+prob+"\">"+prob+"</option>");
  }
  $("#attempt-problem").html(opts);
  $("#clarification-problem").html(opts);
}

function init() {
  $.get("status",null,function(resp) {
    svstatus = resp;
    var langstmp = lang_table(svstatus.languages);
    svstatus.limits = limits_table(svstatus.languages,svstatus.problems);
    svstatus.languages = langstmp;
    init_problems();
    $("#fullname").html(resp.fullname);
    remaining_time_json = {
      remaining_time: resp.rem_time,
      init_time: new Date().getTime()/1000
    };
    var upd_rem_time = function() {
      var now = new Date().getTime()/1000;
      var dt = now-remaining_time_json.init_time;
      var tmp = Math.round(Math.max(0,remaining_time_json.remaining_time-dt));
      var ans = (tmp == 0 ? "The contest is not running." : "Remaining time: " + (tmp+"").toHHMMSS());
      $("#remaining-time").html(ans);
    };
    upd_rem_time();
    setInterval(upd_rem_time, 1000);
    submission();
  });
}

function balloon_img(p) {
  return "<img src=\"balloon.svg\" class=\"svg balloon "+p+"\" />";
}

function verdict_tolongs(verd,p) {
  switch (verd) {
    case      "AC": return "Accepted";
    case      "CE": return "Compile Error";
    case     "RTE": return "Runtime Error";
    case     "TLE": return "Time Limit Exceeded";
    case     "MLE": return "Memory Limit Exceeded";
    case      "WA": return "Wrong Answer";
    case      "PE": return "Presentation Error";
    case   "blind": return "Blind Attempt";
    case "tojudge": return "Not Answered Yet";
    case   "first": return balloon_img(p);
  }
  return "";
}

function source(id,problem,answer) {
  $("#content").html("<h2>Attempt "+id+"</h2>");
  $("#content").append($(
    "<center>"+
      "Problem: "+problem+"<br>"+
      "Answer: "+verdict_tolongs(answer,problem)+
    "</center>"
  ));
  $.get("source/"+id,null,function(resp) {
    $("#content").append($(
      "<pre id=\"code\" class=\"prettyprint linenums\">"+
      "</pre>"
    ));
    $("#code").text(resp);
    PR.prettyPrint();
  });
}

function submission() {
  $("#content").html($("#submission").html());
  $("#attempt-problem").focus();
  $("#attempt-language").html(svstatus.languages);
  $("#limits").html(svstatus.limits);
  $.get("runlist",null,function(resp) {
    var html =
      "<h2>Attempts</h2>"+
      "<table id=\"attempts-table\" class=\"data\">"+
        "<tr>"+
          "<th>#</th>"+
          "<th>Problem</th>"+
          "<th>When</th>"+
          "<th>Language</th>"+
          "<th>Answer</th>"+
        "</tr>"
    ;
    for (var i = 0; i < resp.length; i++) {
      html +=
        "<tr>"+
          "<td><a href=\"#\" onclick=\"source("+resp[i].id+",'"+resp[i].problem+"','"+resp[i].answer+"')\">"+resp[i].id+"</a></td>"+
          "<td>"+resp[i].problem+"</td>"+
          "<td>"+resp[i].time+"</td>"+
          "<td>"+resp[i].language+"</td>"+
          "<td>"+verdict_tolongs(resp[i].answer,resp[i].problem)+"</td>"+
        "</tr>"
      ;
    }
    html +=
      "</table>"
    ;
    $("#runlist").html(html);
    fix_balloons();
  });
}

function scoreboard() {
  $.get("scoreboard",null,function(ans) {
    var resp = ans.scoreboard;
    var html =
      "<h2>Scoreboard"+(ans.frozen ? " (frozen)" : "")+"</h2>"+
      "<table class=\"data\">"+
        "<tr><th>#</th><th>Name</th>"
    ;
    for (var i = 0; i < svstatus.problems.length; i++) {
      html += "<th>"+svstatus.problems[i].name+"</th>";
    }
    html += "<th>Score</th></tr>";
    for (var i = 0; i < resp.length; i++) {
      html +=
        "<tr>"+
          "<td>"+(i+1)+"</td>"+
          "<td>"+resp[i].fullname+"</td>"
      ;
      for (var j = 0; j < resp[i].problems.length; j++) {
        html +=
          "<td class=\"problem\">"
        ;
        var p = resp[i].problems[j];
        if (p.cnt > 0) {
          html += balloon_img(svstatus.problems[j].name)+p.cnt+"/"+p.time;
        }
        else if (p.cnt < 0) html += (-p.cnt)+"/-";
        html +=
          "</td>"
        ;
      }
      html +=
          "<td>"+resp[i].solved+" ("+resp[i].penalty+")</td>"+
        "</tr>"
      ;
    }
    html +=
      "</table>"
    ;
    $("#content").html(html);
    fix_balloons();
  });
}

function clarifications() {
  $.get("clarifications",null,function(resp) {
    var text = 
      "<table class=\"data\">"+
        "<tr><th>Problem</th><th>Question</th><th>Answer</th></tr>"
    ;
    for (var i = 0; i < resp.length; i++) {
      text +=
        "<tr>"+
          "<td>"+resp[i].problem+"</td>"+
          "<td>"+resp[i].question+"</td>"+
          "<td>"+resp[i].answer+"</td>"+
        "</tr>"
      ;
    }
    $("#content").html($("#clarifications").html()+text);
    $("#clarification-problem").focus();
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
  document.getElementById("response").innerHTML = "Waiting server response...";
  var prob = document.getElementById("attempt-problem");
  if (prob.value == "") {
    document.getElementById("response").innerHTML = "Choose a problem!";
    prob.focus();
    return;
  }
  var lang = document.getElementById("attempt-language");
  if (lang.value == "") {
    document.getElementById("response").innerHTML = "Choose a language!";
    lang.focus();
    return;
  }
  var file = document.getElementById("attempt-file");
  var code = document.getElementById("attempt-code");
  if (file.value == "" && code.value == "") {
    document.getElementById("response").innerHTML = "Browse a file or paste a code!";
    file.focus();
    return;
  }
  if (file.value != "" && code.value != "") {
    document.getElementById("response").innerHTML = "Should I send the file or the code?";
    return;
  }
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
  xmlhttp.setRequestHeader("File-name", prob.value+lang.value);
  if (file.value != "") {
    xmlhttp.setRequestHeader("File-size", file.files[0].size);
    xmlhttp.send(file.files[0]);
  }
  else {
    xmlhttp.setRequestHeader("File-size", code.value.length);
    xmlhttp.send(code.value);
  }
  prob.value = "";
  lang.value = "";
  file.value = "";
  code.value = "";
}

function question() {
  var prob = document.getElementById("clarification-problem");
  if (prob.value == "") {
    document.getElementById("response").innerHTML = "Choose a problem!";
    prob.focus();
    return;
  }
  ques = document.getElementById("question");
  if (ques.value == "") {
    document.getElementById("response").innerHTML = "Write something!";
    ques.focus();
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
