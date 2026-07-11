import { readdir, readFile } from "node:fs/promises";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const spritesRoot = path.join(root, "sprites");
const resolution = 2;
const maxTextureDimension = 8192;
const maxGitHubFileBytes = 100 * 1024 * 1024;
const failures = [];

function label(file) {
    return path.relative(root, file).replaceAll("\\", "/");
}

function check(condition, message) {
    if (!condition) {
        failures.push(message);
    }
}

async function walk(directory) {
    const entries = await readdir(directory, { withFileTypes: true });
    const files = [];
    for (const entry of entries) {
        const fullPath = path.join(directory, entry.name);
        if (entry.isDirectory()) {
            files.push(...await walk(fullPath));
        } else if (entry.isFile()) {
            files.push(fullPath);
        }
    }
    return files;
}

async function readPng(file) {
    const data = await readFile(file);
    const signature = "89504e470d0a1a0a";
    check(data.length >= 29, `${label(file)} is too short to be a PNG`);
    if (data.length < 29) {
        return null;
    }
    check(data.subarray(0, 8).toString("hex") === signature, `${label(file)} has an invalid PNG signature`);
    check(data.subarray(12, 16).toString("ascii") === "IHDR", `${label(file)} has no leading IHDR chunk`);
    return {
        width: data.readUInt32BE(16),
        height: data.readUInt32BE(20),
        bitDepth: data[24],
        colorType: data[25],
        bytes: data.length
    };
}

function geometryValues(frame) {
    return [
        frame.frame?.x,
        frame.frame?.y,
        frame.frame?.w,
        frame.frame?.h,
        frame.spriteSourceSize?.x,
        frame.spriteSourceSize?.y,
        frame.spriteSourceSize?.w,
        frame.spriteSourceSize?.h,
        frame.sourceSize?.w,
        frame.sourceSize?.h
    ];
}

const spriteFiles = await walk(spritesRoot);
const pngFiles = spriteFiles.filter((file) => file.endsWith(".png"));
const coverImage = path.join(root, "Persian-CoverImage.png");
pngFiles.push(coverImage);

const pngInfo = new Map();
for (const file of pngFiles) {
    const info = await readPng(file);
    if (!info) {
        continue;
    }
    pngInfo.set(file, info);
    check(info.bitDepth === 8, `${label(file)} must be 8-bit, found ${info.bitDepth}-bit`);
    check(info.colorType === 6, `${label(file)} must be RGBA, found PNG color type ${info.colorType}`);
    check(info.width <= maxTextureDimension && info.height <= maxTextureDimension,
        `${label(file)} is ${info.width}x${info.height}, exceeding the ${maxTextureDimension}px texture ceiling`);
    check(info.bytes < maxGitHubFileBytes,
        `${label(file)} is ${(info.bytes / 1024 / 1024).toFixed(1)} MiB, exceeding GitHub's file limit`);
    check(!path.basename(file).includes("-topaz"), `${label(file)} is a source export, not a publishable texture`);
}

const atlasFiles = spriteFiles.filter((file) => file.endsWith(".json") && !file.endsWith("@2x.json"));
const webAtlasFiles = new Set(spriteFiles.filter((file) => file.endsWith("@2x.json")));
let frameCount = 0;

for (const atlasFile of atlasFiles) {
    let atlas;
    let webAtlas;
    const webAtlasFile = atlasFile.slice(0, -5) + "@2x.json";
    try {
        atlas = JSON.parse(await readFile(atlasFile, "utf8"));
    } catch (error) {
        failures.push(`${label(atlasFile)} is invalid JSON: ${error.message}`);
        continue;
    }
    check(webAtlasFiles.has(webAtlasFile), `${label(atlasFile)} is missing its @2x browser atlas`);
    if (!webAtlasFiles.has(webAtlasFile)) {
        continue;
    }
    try {
        webAtlas = JSON.parse(await readFile(webAtlasFile, "utf8"));
    } catch (error) {
        failures.push(`${label(webAtlasFile)} is invalid JSON: ${error.message}`);
        continue;
    }

    check(Number(atlas.meta?.scale) === resolution, `${label(atlasFile)} must declare scale ${resolution}`);
    check(JSON.stringify(atlas.frames) === JSON.stringify(webAtlas.frames),
        `${label(webAtlasFile)} frame data differs from its native atlas`);
    check(Number(webAtlas.meta?.scale) === resolution, `${label(webAtlasFile)} must declare scale ${resolution}`);

    const imageName = atlas.meta?.image;
    check(typeof imageName === "string" && !imageName.includes("?"), `${label(atlasFile)} has an invalid image path`);
    if (typeof imageName !== "string" || imageName.includes("?")) {
        continue;
    }
    check(webAtlas.meta?.image === `${imageName}?resolution=@2x`,
        `${label(webAtlasFile)} must mark its image URL as @2x`);

    const imageFile = path.join(path.dirname(atlasFile), imageName);
    const info = pngInfo.get(imageFile);
    check(Boolean(info), `${label(atlasFile)} references missing image ${imageName}`);
    if (!info) {
        continue;
    }
    check(atlas.meta?.size?.w === info.width && atlas.meta?.size?.h === info.height,
        `${label(atlasFile)} metadata is ${atlas.meta?.size?.w}x${atlas.meta?.size?.h}, image is ${info.width}x${info.height}`);
    check(webAtlas.meta?.size?.w === info.width && webAtlas.meta?.size?.h === info.height,
        `${label(webAtlasFile)} metadata does not match ${imageName}`);

    for (const [name, frame] of Object.entries(atlas.frames ?? {})) {
        frameCount++;
        const prefix = `${label(atlasFile)} frame ${name}`;
        const values = geometryValues(frame);
        const hasNumericGeometry = values.every(Number.isFinite);
        check(hasNumericGeometry, `${prefix} contains non-numeric geometry`);
        if (!hasNumericGeometry) {
            continue;
        }
        check(values.every((value) => value >= 0), `${prefix} contains negative geometry`);
        check(values.every((value) => Number.isInteger(value / resolution)),
            `${prefix} does not map cleanly to ${resolution}x logical pixels`);
        check(frame.frame.x + frame.frame.w <= info.width && frame.frame.y + frame.frame.h <= info.height,
            `${prefix} extends beyond ${imageName}`);
        check(frame.rotated === false, `${prefix} is rotated, which the native renderers do not support`);
    }
}

const expectedWebAtlases = new Set(atlasFiles.map((file) => file.slice(0, -5) + "@2x.json"));
for (const webAtlasFile of webAtlasFiles) {
    check(expectedWebAtlases.has(webAtlasFile), `${label(webAtlasFile)} has no matching native atlas`);
}

if (failures.length > 0) {
    console.error(`Asset validation failed with ${failures.length} issue(s):`);
    for (const failure of failures) {
        console.error(`- ${failure}`);
    }
    process.exitCode = 1;
} else {
    console.log(`Validated ${pngFiles.length} RGBA8 PNGs, ${atlasFiles.length} atlas pairs, and ${frameCount} frames.`);
}
