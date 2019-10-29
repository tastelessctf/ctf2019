import * as express from "express";
import * as http from "http";
import * as bodyParser from "body-parser";
import * as cookieParser from "cookie-parser";
import * as WebSocket from "ws";
import * as csp from "helmet-csp";
import * as crypto from "crypto";
import * as express_handlebars from "express-handlebars";
import * as namegen from "unique-names-generator";
import * as kue from "kue";
import * as prom from "prom-client";
import * as Recaptcha from 'express-recaptcha';

const collectDefaultMetrics = prom.collectDefaultMetrics;
collectDefaultMetrics({ prefix: 'gabbr_' });

const requests = new prom.Counter({
  name: 'gabbr_requests',
  help: 'Requests',
  labelNames: ['method', 'endpoint'],
});

const chat_rooms = new prom.Gauge({
	name: 'gabbr_chat_rooms',
	help: 'Number of chat rooms',
});

const connections = new prom.Gauge({
  name: 'gabbr_connections',
  help: 'Number of connected clients',
});

const messages = new prom.Counter({
  name: 'gabbr_messages',
  help: 'Number of messages sent',
});

const reports = new prom.Counter({
  name: 'gabbr_reports',
  help: 'Number of reports',
  labelNames: ['status'],
})

const recaptcha_failed = new prom.Counter({
  name: 'gabbr_recaptcha_failed',
  help: 'Number of invalid recaptchas',
})

const csp_violations = new prom.Counter({
  name: 'gabbr_csp_violations',
  help: 'Number of CSP violations reported',
});

const queue = kue.createQueue({
    redis: process.env.REDIS || "redis://localhost:6379"
});

const port = process.env.PORT || 8999;
const host = process.env.HOST || `localhost:${port}`;
const recaptchaPublicKey = process.env.RECAPTCHA_PUBLIC_KEY || "abc";
const recaptchaPrivateKey = process.env.RECAPTCHA_PRIVATE_KEY || "def"

const recaptcha = new Recaptcha.RecaptchaV2(recaptchaPublicKey, recaptchaPrivateKey);

const app = express();
app.engine("handlebars", express_handlebars());
app.set("view engine", "handlebars");

interface ExtendedServerResponse extends http.ServerResponse {
  nonce: string;
}

interface ExtendedResponse extends express.Response {
  nonce: string;
}

interface UserSocket extends WebSocket {
  username: string;
}

app.use(bodyParser.json({ type: "application/json" }));
app.use(bodyParser.json({ type: "application/csp-report" }));
app.use(bodyParser.urlencoded());
app.use(cookieParser());

const name_config: namegen.UniqueNamesGeneratorConfig = {
  separator: " ",
  length: 2
};

app.use(function(req, res, next) {
  // check if client sent cookie
  var cookie = req.cookies.username;
  if (cookie === undefined) {
    res.cookie("username", namegen.uniqueNamesGenerator(name_config), {
      maxAge: 900000,
      httpOnly: false
    });
  }
  next(); // <-- important!
});

app.use(function(_req: http.IncomingMessage, res: http.ServerResponse, next) {
  (res as ExtendedServerResponse).nonce = crypto
    .randomBytes(12)
    .toString("hex");
  next();
});

app.use(
  csp({
    // Specify directives as normal.
    directives: {
      defaultSrc: ["'self'"],
      scriptSrc: [
        (_req, res) => `'nonce-${(res as ExtendedServerResponse).nonce}'` // 'nonce-348c18b14aaf3e00938d8bdd613f1149'
      ],
      frameSrc: ["https://www.google.com/recaptcha/"],
      connectSrc: ["'self'", "xsstest.tasteless.eu", "https://www.google.com/recaptcha/"],
      workerSrc: ["https://www.google.com/recaptcha/"],
      styleSrc: ["'unsafe-inline'", "https://www.gstatic.com/recaptcha/"],
      fontSrc: ["'self'"],
      imgSrc: ["*"],
      //sandbox: ["allow-forms", "allow-scripts"],
      reportUri: "https://xsstest.ctf.tasteless.eu/report-violation",
      objectSrc: ["'none'"],
      upgradeInsecureRequests: false,
    },

    // This module will detect common mistakes in your directives and throw errors
    // if it finds any. To disable this, enable "loose mode".
    loose: false,

    // Set to true if you only want browsers to report errors, not block them.
    // You may also set this to a function(req, res) in order to decide dynamically
    // whether to use reportOnly mode, e.g., to allow for a dynamic kill switch.
    reportOnly: false,

    // Set to true if you want to blindly set all headers: Content-Security-Policy,
    // X-WebKit-CSP, and X-Content-Security-Policy.
    setAllHeaders: false,

    // Set to true if you want to disable CSP on Android where it can be buggy.
    disableAndroid: false,

    // Set to false if you want to completely disable any user-agent sniffing.
    // This may make the headers less compatible but it will be much faster.
    // This defaults to `true`.
    browserSniff: true
  })
);

app.get("/", (req, res) => {
  requests.inc({endpoint: "/", method: "GET"});
  res.render("home", {
    nonce: (res as ExtendedResponse).nonce,
    host: req.headers.host,
    recaptchaPublicKey: recaptchaPublicKey,
    layout: false
  });
});

// app.post("/report/:uuid", recaptcha.middleware.verify, (req, res) => {
// recaptcha disabled
app.post("/report/:uuid", (req, res) => {
  requests.inc({endpoint: "/report/:uuid", method: "POST"});
  console.log(req.params.uuid);
  // if (req.recaptcha !== undefined && !req.recaptcha.error) {
  if (true) {
    reports.inc({'status': 'requested'});
    queue.create("report", {
      uuid: req.params.uuid
    }).save( function(err: any){
      if( err ) {
        console.log( `failed to enqueue ${req.params.uuid}: ${err}`)
        reports.inc({'status': 'failed'});
        res.sendStatus(500);
    }});
  } else {
    recaptcha_failed.inc();
    reports.inc({'status': 'failed'});
    res.sendStatus(403);
  }
  reports.inc({'status': 'enqueued'});
  res.sendStatus(200);
});

app.use("/static", express.static("public"));

app.post("/report-violation", (req, res) => {
  requests.inc({endpoint: "/report-violation", method: "POST"});
  csp_violations.inc();
  console.log(req.body);
  res.sendStatus(200);
});

app.get("/metrics", (req,res) => {
  res.set('Content-Type', prom.register.contentType);
  res.end(prom.register.metrics());
})

//initialize a simple http server
const server = http.createServer(app);

//initialize the WebSocket server instance
const wss = new WebSocket.Server({ server });

let chats: { [id: string]: WebSocket[] } = {};

wss.on("connection", (ws: WebSocket, req) => {
  if (req.url === undefined) {
    ws.close(1008);
    return;
  }
  let uuid = req.url.slice(1);
  console.log(`connected ${uuid}`);

  let index = 0;
  if (uuid in chats) {
    index = chats[uuid].push(ws) - 1;
  } else {
    chat_rooms.inc();
    chats[uuid] = [ws];
  }
  connections.inc();

  //connection is up, let's add a simple simple event
  ws.on("message", (message: string) => {
    console.log("received: %s", message);
    messages.inc();
    let data = JSON.parse(message);
    data.useragent = req.headers["user-agent"];
    (ws as UserSocket).username = data.username;
    chats[uuid].forEach(peer => {
      peer.send(JSON.stringify(data));
    });
  });

  ws.on("close", connection => {
    console.log(`disconnected ${uuid}`);
    if (uuid !== undefined) chats[uuid].splice(index, 1);
    connections.dec();
    var usersock = ws as UserSocket;
    if (usersock != null) {
      chats[uuid].forEach(peer => {
        peer.send(
          JSON.stringify({
            username: usersock.username,
            type: "gabbr-leave"
          })
        );
      });
    }
    if (chats[uuid] === undefined || chats[uuid].length == 0) {
      chat_rooms.dec();
      delete chats[uuid];
    }
  });
});

//start our server
server.listen(port, () => {
  console.log(`Server started on port ${port} :)`);
});
