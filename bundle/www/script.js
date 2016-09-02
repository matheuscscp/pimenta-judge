psize = 20; // page size

$(document).ready(function() {
  init();
  contests();
});

// =============================================================================
// GET
// =============================================================================

function init() {
  $.ajax({
    url: "status",
    success: function(resp) {
      svdt = new Date().getTime() - 1000*resp.time;
      username = resp.name;
      $("#username").text(resp.name);
      var html =
        "<li><a href=\"#\" onclick=\"contests()\">Contests</a></li>\n"+
        "<li><a href=\"#\" onclick=\"problems()\">Problems</a></li>\n"
      ;
      if (username != "") html +=
        "<li><a href=\"#\" onclick=\"attempts()\">Attempts</a></li>\n"
      ;
      html +=
        "<li><a href=\"#\" onclick=\"users()\">Users</a></li>\n"
      ;
      if (username == "") html +=
        "<li><a href=\"login.html\">Login</a></li>"
      ;
      else html +=
        "<li><a href=\"logout\">Logout</a></li>"
      ;
      $(".menu").html(html);
    },
    async: false
  });
}

function login() {
  window.location = "/login.html";
}

function contests() {
  $.get("attempts",null,function(atts) {
    setup_solved(atts);
    $.get("contests",null,function(resp) {
      for (var i = 0; i < resp.length; i++) {
        var obj = resp[i];
        var s = obj.start;
        var tmp = new Date(s.year,s.month-1,s.day,s.hour,s.minute,0,0);
        obj.start = Math.round((tmp.getTime()+svdt)/60000)*60000;
      }
      resp.sort(function(a,b) {
        if (a.start != b.start) return b.start-a.start;
        return b.id-a.id;
      });
      $("#c1").html("<h2>Contests</h2><div id=\"pages\"></div>");
      var headers = [
        id_header("contest"),
        name_header("contest"),
        {name: "Start", field: function(obj) {
          return new Date(obj.start).toString();
        }},
        {name: "Duration", field: "duration"},
        {name: "Freeze", field: "freeze"},
        {name: "Blind", field: "blind"}
      ];
      if (username != "") headers.push({
        name: "Solved",
        field: function(obj) {
          var p = obj.problems;
          var n = p.length, cnt = 0;
          for (var i = 0; i < n; i++) {
            if (p[i] in solved && "solved" in solved[p[i]]) cnt++;
          }
          var ans = cnt+"/"+n;
          if (cnt < n) return ans;
          return "<span class=\"solved\">"+ans+"</span>";
        }
      })
      render_pages("#pages",headers,resp,function(obj,tr) {
        if (tr.find("span.solved").length > 0) {
          tr.removeClass("zebra0");
          tr.removeClass("zebra1");
          tr.addClass("solved");
          tr.attr("title","Solved");
        }
      });
    });
  });
}

function problems() {
  $.get("attempts",null,function(atts) {
    setup_solved(atts);
    $.get("problems",null,function(resp) {
      resp.sort(function(a,b) { return b.id-a.id; });
      $("#c1").html("<h2>Problems</h2><div id=\"pages\"></div>");
      render_pages(
        "#pages",[id_header("problem"),name_header("problem")],resp,check_solved
      );
    });
  });
}

function attempts() {
  if (username == "") { login(); return; }
  $.get("attempts",null,function(resp) {
    resp.sort(function(a,b) {
      var x = time(a.when), y = time(b.when);
      if (x != y) return y-x;
      return b.id-a.id;
    });
    $("#c1").html("<h2>Attempts</h2><div id=\"pages\"></div>");
    render_pages("#pages",[
      id_header("attempt"),
      {name: "Problem", field: function(obj) {
        var ans = "<a href=\"#\" onclick=\"problem(";
        ans += obj.problem.id;
        ans += ")\">"+obj.problem.id+" — "+obj.problem.name+"</a>";
        return ans;
      }},
      {name: "When", field: function(obj) {
        return timestamp(obj.when);
      }},
      {name: "Language", field: "language"},
      {name: "Verdict", field: function(obj) {
        return verdict(obj);
      }}
    ],resp);
  });
}

function users() {
  $.get("users",null,function(resp) {
    $("#c1").html("<h2>Users</h2><div id=\"pages\"></div>");
    render_pages("#pages",[
      id_header("user"),
      name_header("user"),
      {name: "Solved", field: "solved"},
      {name: "Tried", field: "tried"}
    ],resp);
  });
}

function contest(id) {
  $.get("contest/"+id,null,function(resp) {
    var html =
      "<h2>Contest "+resp.id+" — "+resp.name+"</h2>"+
      "<div class=\"center inner-menu\">"+
        "<a href=\"#\" onclick=\"contest_problems("+id+")\">Problems</a> "
    ;
    if (username != "") html +=
        "<a href=\"#\" onclick=\"contest_attempts("+id+")\">Attempts</a> "
    ;
    html +=
        "<a href=\"#\" onclick=\"contest_scoreboard("+id+")\">Scoreboard</a>"+
      "</div>"+
      "<div id=\"c2\"></div>"
    ;
    $("#c1").html(html);
    contest_problems(id);
  })
}

function contest_problems(id) {
  $.get("contest/attempts/"+id,null,function(atts) {
    setup_solved(atts);
    $.get("contest/problems/"+id,null,function(resp) {
      var tbl = $("<table id=\"contest-problems\"></table>");
      for (var i = 0; i < resp.length; i++) {
        var id = resp[i].id;
        var c = upletter(i);
        var name = resp[i].name;
        var html = "<tr><td><a href=\"#\" onclick=\"problem("+id+")\">";
        html += c+" — "+name+"</a>";
        html += "</td></tr>";
        var tmp = $(html);
        check_solved(resp[i],tmp);
        tbl.append(tmp);
      }
      $(content()).html("<h2>Problems</h2>").append(tbl);
    });
  });
}

function contest_attempts(id) {
  if (username == "") { login(); return; }
  $.get("contest/attempts/"+id,null,function(resp) {
    sort_contest_attempts(resp);
    var set = {};
    for (var i = 0; i < resp.length; i++) {
      if (resp[i].status != "judged" || resp[i].verdict != "AC") continue;
      if (resp[i].problem.id in set) continue;
      set[resp[i].problem.id] = resp[i].id;
      resp[i].verdict = "first";
    }
    $(content()).html("<h2>Attempts</h2><div id=\"pages\"></div>");
    render_pages("#pages",[
      {name: "#", field: function(obj) {
        return "<a href=\"#\" onclick=\"attempt("+obj.id+")\">"+obj.id+"</a>";
      }},
      {name: "Problem", field: function(obj) {
        var ans = "<a href=\"#\" onclick=\"problem(";
        ans += obj.problem.id+")\">";
        ans += upletter(obj.problem.idx)+" — "+obj.problem.name+"</a>";
        return ans;
      }},
      {name: "When", field: "contest_time"},
      {name: "Language", field: "language"},
      {name: "Verdict", field: function(obj) {
        return verdict(obj);
      }}
    ],resp,undefined,render_balloons);
  });
}

function contest_scoreboard(id) {
  $.get("contest/scoreboard/"+id,null,function(resp) {
    var msg = (
      resp.status == "frozen" ? " (frozen at "+resp.freeze+" minutes)" : (
        resp.status == "final" ? " (final)" : ""
      )
    );
    var n = resp.colors.length;
    var sc = compute_scoreboard(n,resp.attempts);
    var html =
      "<h2>Scoreboard"+msg+"</h2>"+
      "<table class=\"data\">"+
        "<tr>"+
          "<th>#</th>"+
          "<th>Name</th>"
    ;
    for (var i = 0; i < n; i++) {
      html +=
          "<th>"+String.fromCharCode(i+65)+"</th>"
      ;
    }
    html +=
          "<th>Score</th>"+
        "</tr>"
    ;
    for (var i = 0; i < sc.length; i++) {
      html +=
        "<tr>"+
          "<td>"+(i+1)+"</td>"+
          "<td>"+sc[i].name+"</td>"
      ;
      for (var j = 0; j < n; j++) {
        var p = sc[i].problems[j];
        var tmp = (
          p.atts > 0 ? balloon(resp.colors[j])+p.atts+"/"+p.time : (
            p.atts < 0 ? (-p.atts)+"/-" : ""
          )
        );
        html +=
          "<td class=\"scoreboard-problem\">"+tmp+"</td>"
        ;
      }
      html +=
          "<td>"+sc[i].score.num+" ("+sc[i].score.time+")</td>"
      ;
    }
    $(content()).html(html);
    $(".scoreboard-problem").css("padding", "2px");
    render_balloons();
  });
}

function problem(id) {
  $.get("problem/"+id,null,function(resp) {
    var languages = resp.languages;
    var limits_table =
      "<table class=\"data\">"+
        "<tr>"+
          "<th>Language</th>"+
          "<th>Time limit</th>"+
          "<th>Memory limit</th>"+
          "<th>Info</th>"+
        "</tr>"
    ;
    for (var i = 0; i < languages.length; i++) {
      limits_table +=
        "<tr>"+
          "<td>"+languages[i].name+"</td>"+
          "<td>"+languages[i].timelimit+" s</td>"+
          "<td>"+languages[i].memlimit+" kB</td>"+
          "<td>"+languages[i].info+"</td>"+
        "</tr>"
      ;
    }
    limits_table +=
      "</table>"
    ;
    var lang_opts =
      "<option></option>"
    ;
    for (var i = 0; i < languages.length; i++) {
      lang_opts +=
      "<option value=\""+languages[i].id+"\">"+languages[i].name+"</option>"
      ;
    }
    var stmnt = "";
    if (resp.has_statement) {
      stmnt = "<embed src=\"problem/statement/"+resp.id+"\">";
    }
    var html =
      "<h2>Problem "+resp.id+" — "+resp.name+"</h2>"+
      "<div id=\"problem\">"+
        limits_table
    ;
    if (username != "") html +=
        "<h4 id=\"response\"></h4>"+
        "<table>"+
          "<tr>"+
            "<td>Language:</td>"+
            "<td><select id=\"attempt-language\">"+lang_opts+"</select></td>"+
          "</tr>"+
          "<tr>"+
            "<td>Browse file...</td>"+
            "<td><input id=\"attempt-file\" type=\"file\"></td>"+
          "</tr>"+
          "<tr>"+
            "<td valign=\"top\">...<b>or</b> paste code:</td>"+
            "<td><textarea id=\"attempt-code\"></textarea></td>"+
          "</tr>"+
          "<tr>"+
            "<td></td>"+
            "<td class=\"right\">"+
              "<button onclick=\"new_attempt("+resp.id+")\">Send</button>"+
            "</td>"+
          "</tr>"+
        "</table>"
    ;
    html +=
        stmnt+
      "</div>"
    ;
    $(content()).html(html);
    $("#attempt-language").focus();
  });
}

function attempt(id) {
  if (username == "") { login(); return; }
  var func = function(resp,cname) {
    var html =
      "<h2>Attempt "+resp.id+"</h2>"+
      "<table class=\"data\">"+
        "<tr>"+
          "<th>Problem</th>"+
          "<td>"+
            "<a href=\"#\" onclick=\"problem("+resp.problem.id+")\">"+
              resp.problem.id+" — "+resp.problem.name+
            "</a>"+
          "</td>"+
        "</tr>"+
        "<tr><th>When</th><td>"+timestamp(resp.when)+"</td></tr>"+
        "<tr><th>Language</th><td>"+resp.language+"</td></tr>"+
        "<tr><th>Verdict</th><td>"+verdict(resp)+"</td></tr>"
    ;
    if ("contest_time" in resp) html +=
        "<tr>"+
          "<th>Contest</th>"+
          "<td>"+
            "<a href=\"#\" onclick=\"contest("+resp.contest+")\">"+cname+"</a>"+
          "</td>"+
        "</tr>"+
        "<tr><th>Contest Time</th><td>"+resp.contest_time+" minutes</td></tr>"
    ;
    html +=
      "</table>"
    ;
    $(content()).html(html)
    .append($(
      "<pre id=\"code\" class=\"prettyprint linenums\">"+
      "</pre>"
    ))
    .find("#code").text(resp.source);
    PR.prettyPrint();
  };
  $.get("attempt/"+id,null,function(resp) {
    if (!("contest" in resp)) { func(resp); return; }
    $.get("contest/"+resp.contest,null,function(resp2) {
      func(resp,resp2.name);
    });
  });
}

function user(id) {
  $.get("user/"+id,null,function(resp) {
    $("#c1").html(
      "<h2>"+resp.name+"'s solved problems</h2><div id=\"pages\"></div>"
    );
    render_pages("#pages",[
      id_header("problem"),
      name_header("problem","Problem")
    ],resp.solved);
  });
}

// =============================================================================
// POST
// =============================================================================

function do_login() {
  if (username != "") window.location = "/";
  var user = $("#user");
  var pass = $("#pass");
  if (user.val() == "" || pass.val() == "") {
    $("#response").html("Invalid username/password!");
    return;
  }
  $.post("login",JSON.stringify({
    username: user.val(),
    password: pass.val()
  }),function(resp) {
    if (resp == "ok") window.location = "/";
    else $("#response").html(resp);
  });
  user.val("");
  pass.val("");
}

function new_attempt(probid) {
  if (username == "") { login(); return; }
  var lang = $("#attempt-language");
  if (lang.val() == "") {
    $("#response").html("Choose a language!");
    lang.focus();
    return;
  }
  var file = document.getElementById("attempt-file");
  var code = $("#attempt-code");
  if ((file.value == "") == (code.val() == "")) {
    $("#response").html("Browse a file or paste a code!");
    file.focus();
    return;
  }
  var xhr = Ajax();
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      $("#response").html(xhr.responseText);
    }
  };
  xhr.open("POST","new_attempt/"+probid+"/"+lang.val(),true);
  xhr.send(file.value == "" ? code.val() : file.files[0]);
  $("#response").html("Waiting server response...");
  lang.val("");
  file.value = "";
  code.val("");
}

// =============================================================================
// generic
// =============================================================================

function content() {
  var ans = $("#c2");
  if (ans.length > 0) return "#c2";
  return "#c1";
}

function sort_contest_attempts(atts) {
  atts.sort(function(a,b) {
    if (a.contest_time != b.contest_time) return a.contest_time-b.contest_time;
    return a.id-b.id;
  });
}

function compute_scoreboard(nprobs,atts) {
  sort_contest_attempts(atts);
  var new_user = function(name) {
    var ans = {name: name, problems: [], ACs: [], score: {num: 0, time: 0}};
    for (var i = 0; i < nprobs; i++) ans.problems.push({atts: 0, time: 0});
    return ans;
  };
  var users = {};
  for (var i = 0; i < atts.length; i++) {
    var att = atts[i];
    if (!(att.user in users)) users[att.user] = new_user(att.user);
    var us = users[att.user];
    var p = us.problems[att.problem];
    if (p.atts > 0) continue;
    if (att.verdict != "AC") p.atts--;
    else {
      p.atts = 1-p.atts;
      p.time = att.contest_time;
      us.ACs.push(att.contest_time);
      us.score.num++;
      us.score.time += 20*(p.atts-1)+p.time;
    }
  }
  var ans = [];
  for (us in users) if ("problems" in users[us]) ans.push(users[us]);
  ans.sort(function(a,b) {
    if (a.score.num != b.score.num) return b.score.num-a.score.num;
    if (a.score.time != b.score.time) return a.score.time-b.score.time;
    for (var i = a.score.num-1; 0 <= i; i--) {
      if (a.ACs[i] != b.ACs[i]) return a.ACs[i]-b.ACs[i];
    }
    if (a.name < b.name) return -1;
    if (a.name > b.name) return  1;
    return 0;
  });
  return ans;
}

function timestamp(svtime) {
  return new Date(time(svtime)).toString();
}

function time(svtime) {
  return 1000*svtime+svdt;
}

function setup_solved(atts) {
  solved = {};
  tried = {};
  for (var i = 0; i < atts.length; i++) {
    tried[atts[i].problem.id] = {tried: true};
    if (atts[i].verdict == "AC") {
      solved[atts[i].problem.id] = {solved: true};
    }
  }
}
function check_solved(obj,tr) {
  if (obj.id in solved && "solved" in solved[obj.id]) {
    tr.removeClass("zebra0");
    tr.removeClass("zebra1");
    tr.addClass("solved");
    tr.attr("title","Solved");
  }
  else if (obj.id in tried && "tried" in tried[obj.id]) {
    tr.removeClass("zebra0");
    tr.removeClass("zebra1");
    tr.addClass("tried");
    tr.attr("title","Tried");
  }
}

function upletter(idx) {
  return String.fromCharCode(65+idx);
}

function render_balloons() {
  $.get("balloon.svg",null,function(resp) {
    var svg = $(resp).find("svg");
    $(".balloon").each(function() {
      var tsvg = svg.clone();
      var bdiv = $(this);
      tsvg.children("path.balloonfill").css("fill",bdiv.attr("data-color"));
      bdiv.replaceWith(tsvg);
    });
  });
}
function balloon(color) {
  return "<div class=\"balloon\" data-color=\""+color+"\"></div>";
}
function verdict(att) {
  if (att.status != "judged") att.verdict = att.status;
  switch (att.verdict) {
    case        "AC": return "Accepted";
    case        "CE": return "Compile Error";
    case       "RTE": return "Runtime Error";
    case       "TLE": return "Time Limit Exceeded";
    case       "MLE": return "Memory Limit Exceeded";
    case        "WA": return "Wrong Answer";
    case        "PE": return "Presentation Error";
    case   "waiting": return "Not Answered Yet";
    case     "blind": return "Blind Attempt";
    case "cantjudge": return "Not Answered Yet";
    case     "first": return balloon(att.problem.color);
  }
  return "";
}

function Ajax() {
  return (
    window.XMLHttpRequest ?
    new XMLHttpRequest() :
    new ActiveXObject("Microsoft.XMLHTTP")
  );
}

function id_header(func) {
  return {
    name: "ID",
    field: function(obj) {
      return "<a href=\"#\" onclick=\""+func+"("+obj.id+")\">"+obj.id+"</a>";
    }
  };
}
function name_header(func,name) {
  if (name === undefined) name = "Name";
  return {
    name: name,
    field: function(obj) {
      return "<a href=\"#\" onclick=\""+func+"("+obj.id+")\">"+obj.name+"</a>";
    }
  };
}
function render_page(page) {
  var cols = pages.header.length, rows = pages.data.length;
  var tbody = $(pages.where+" tbody").html("");
  var j = 0, zebra = 1;
  for (var i = page*psize; i < rows && j < psize; i++, j++, zebra = 1-zebra) {
    var obj = pages.data[i];
    var html = "<tr class=\""+(zebra ? "zebra1" : "zebra0")+"\">";
    for (var k = 0; k < cols; k++) {
      var tmp = pages.header[k].field;
      var ans;
      if (typeof tmp === 'string' || tmp instanceof String) {
        if (tmp == "position") ans = i+1;
        else ans = obj[tmp];
      }
      else ans = tmp(obj);
      html += "<td>"+ans+"</td>";
    }
    html += "</tr>";
    var tr = $(html);
    pages.rowcb(obj,tr);
    tbody.append(tr);
  }
  for (; j < psize; j++) {
    var html = "<tr>";
    for (var k = 0; k < cols; k++) html += "<td></td>";
    html += "</tr>";
    tbody.append($(html));
  }
  $(pages.where+" tfoot td").html("")
  .append($(
    "<a"+(page == 0 ? " class=\"hidden\"" : "")+
    " href=\"#\" onclick=\"render_page("+(page-1)+")\">Prev</a>"
  ))
  .append($(
    "<a"+(rows <= (page+1)*psize ? " class=\"hidden\"" : "")+
    " href=\"#\" onclick=\"render_page("+(page+1)+")\">Next</a>"
  ));
  pages.poscb();
}
function render_pages(where, header, data, rowcb, poscb) {
  pages = {
    where: where,
    header: header,
    data: data,
    rowcb: (rowcb === undefined ? function(){} : rowcb),
    poscb: (poscb === undefined ? function(){} : poscb)
  };
  var html =
    "<table class=\"data\">"+
      "<thead>"+
        "<tr>"
  ;
  for (var i = 0; i < header.length; i++) {
    html +=
          "<th>"+header[i].name+"</th>"
    ;
  }
  html +=
        "</tr>"+
      "</thead>"+
      "<tfoot><td colspan=\""+header.length+"\"></td></tfoot>"+
      "<tbody></tbody>"+
    "</table>"
  ;
  $(where).html(html);
  render_page(0);
}
