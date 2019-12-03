let data = '';

if (!process.argv[2]) {
    console.error('Missing argument (filename). Usage: node hex-to-buffer.js output-file');
    process.exit(1);
}

process.stdin.resume();
process.stdin.setEncoding('binary');

process.stdin.on('data', chunk => {
    data += chunk;
});

process.stdin.on('end', go);

let stdinTimeout = setTimeout(() => {
    console.warn('No data received on stdin... Shutting down.');
    process.exit(1);
}, 2000);

function go() {
    clearTimeout(stdinTimeout);

    data = Buffer.from(data, 'hex');

    require('fs').writeFileSync(process.argv[2], data);
    console.log('Written to', process.argv[2]);
}
