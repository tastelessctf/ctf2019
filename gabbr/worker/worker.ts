import * as kue from "kue";
import * as selenium from "selenium-webdriver";
import * as firefox from "selenium-webdriver/firefox";
import * as prom from "prom-client";
import * as sleep from "sleep";

const url = process.env.URL || "http://localhost:8999/";
const username = process.env.USERNAME || "admin";
const flag = process.env.FLAG || "tctf{flag_goes_here}";
const worker_id = process.env.WORKER_ID || "default"

const gateway = new prom.Pushgateway(process.env.PUSHGATEWAY || "https://pushgateway:9091");
const execution_time = new prom.Histogram({
  name: 'gabbr_worker_execution_time',
  help: 'Worker Execution Time'
});

const executions = new prom.Counter({
  name: 'gabbr_worker_executions',
  help: 'Number of executions',
  labelNames: ['state']
})

const collectDefaultMetrics = prom.collectDefaultMetrics;
collectDefaultMetrics({ prefix: 'gabbr_' });

async function check_page(uuid: string) {
  const options = new firefox.Options();
  options.headless();
  let driver = await new selenium.Builder()
    .forBrowser("firefox")
    .setFirefoxOptions(options)
    .build();
  try {
    await driver.get(`${url}`);
    await driver.manage().addCookie({ name: "username", value: username });
    await driver.manage().addCookie({ name: "flag", value: flag });
    await driver.get(`${url}#${uuid}`);
    sleep.sleep(15);
    let chatbox = await driver.findElement(selenium.By.id("chatbox"));
    await chatbox.sendKeys("<img src=\"https://media.tenor.com/images/44fd110df32a75a7753d0e417ddadb84/tenor.gif\"/>\n");
    executions.inc({state: 'successful'});
  } catch {
    executions.inc({state: 'failed'});
  } finally {
    await driver.quit();
  }
}

const queue = kue.createQueue({
  redis: process.env.REDIS || "redis://localhost:6379"
});

console.log(queue);

queue.process("report", function(
  job: { data: { uuid: string } },
  done: () => void
) {
  console.log(`checking page ${job.data.uuid}`);
  const stopTimer = execution_time.startTimer();
  check_page(job.data.uuid).then(() => {
    gateway.pushAdd({ jobName: worker_id }, function(_err, _resp, _body) {
      console.log(_err);
    });
    stopTimer();
    done();
  });

});
