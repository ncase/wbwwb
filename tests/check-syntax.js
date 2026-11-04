"use strict";

const fs = require("fs");
const path = require("path");
const { spawnSync } = require("child_process");

const root = path.resolve(__dirname, "..");
const searchRoots = [path.join(root, "js"), __dirname];
const files = [];

function walk(directory) {
    for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
        const absolutePath = path.join(directory, entry.name);
        if (entry.isDirectory()) {
            if (absolutePath === path.join(root, "js", "lib")) continue;
            walk(absolutePath);
        } else if (entry.isFile() && entry.name.endsWith(".js")) {
            files.push(absolutePath);
        }
    }
}

for (const directory of searchRoots) walk(directory);
files.sort();

for (const file of files) {
    const result = spawnSync(process.execPath, ["--check", file], { stdio: "inherit" });
    if (result.error) throw result.error;
    if (result.status !== 0) {
        console.error(`Syntax check failed: ${path.relative(root, file)}`);
        process.exit(result.status || 1);
    }
}

console.log(`PASS: ${files.length} non-vendor JavaScript files passed node --check.`);
