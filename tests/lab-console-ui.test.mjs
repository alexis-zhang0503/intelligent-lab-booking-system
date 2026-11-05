import assert from 'node:assert/strict';
import fs from 'node:fs';
import {test} from 'node:test';

const html=fs.readFileSync(new URL('../index.html',import.meta.url),'utf8');
const css=fs.readFileSync(new URL('../lab-console.css',import.meta.url),'utf8');

test('light laboratory console is loaded after the existing interface layers',()=>{
  assert.match(html,/agent-assistant\.css[\s\S]*lab-console\.css/);
  assert.doesNotMatch(css,/color-scheme\s*:\s*dark/);
});

test('secondary engineering copy is visually removed without suppressing controls',()=>{
  assert.match(css,/#kicker,.eyebrow,.sub,.helper/);
  assert.match(css,/\.agent-starters button>small/);
  assert.match(css,/\.primary,.secondary,.outline/);
});
