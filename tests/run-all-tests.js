"use strict";

const fs = require("fs");
const path = require("path");
const { spawnSync } = require("child_process");

const testDirectory = __dirname;
const tests = fs.readdirSync(testDirectory)
    .filter((name) => name.endsWith(".test.js"))
    .sort();

if (tests.length === 0) {
    console.error("No test suites found in tests/.");
    process.exit(1);
}

for (const testFile of tests) {
    const absolutePath = path.join(testDirectory, testFile);
    console.log(`\n=== ${testFile} ===`);
    const result = spawnSync(process.execPath, [absolutePath], { stdio: "inherit" });
    if (result.error) throw result.error;
    if (result.status !== 0) {
        process.exit(result.status || 1);
    }
}

console.log(`\nPASS: ${tests.length} test suites.`);
