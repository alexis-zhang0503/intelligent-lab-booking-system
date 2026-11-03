// The entry page versions this small loader; the loader fetches the current
// release manifest without HTTP caching before importing the app module.
const entry=document.querySelector('script[data-app-version]');
const fallbackVersion=entry?.dataset.appVersion||'20261103-7';
const manifestUrl=new URL('../../version.json',import.meta.url);
let loadedVersion=fallbackVersion;
let checking=false;
async function readVersion(){
  const url=new URL(manifestUrl);url.searchParams.set('_',Date.now());
  const response=await fetch(url,{cache:'no-store',signal:AbortSignal.timeout(5000)});
  if(!response.ok)throw new Error(`Version check failed (${response.status})`);
  const manifest=await response.json();
  if(!/^[a-zA-Z0-9._-]{1,80}$/.test(manifest.version))throw new Error('Invalid release version');
  return manifest.version;
}
async function checkForUpdate(){
  if(checking||document.visibilityState==='hidden')return;
  checking=true;
  try{
    const version=await readVersion();
    if(version!==loadedVersion){
      const notice=document.querySelector('#updateNotice');
      if(notice){notice.hidden=false;document.querySelector('#reloadVersion').onclick=()=>{
        const url=new URL(location.href);url.searchParams.set('v',version);location.replace(url.href);
      }}
    }
  }catch(error){console.warn('Version check unavailable',error.message)}
  finally{checking=false}
}
async function start(){
  try{loadedVersion=await readVersion()}catch(error){console.warn('Using entry-page version',error.message)}
  document.documentElement.dataset.appVersion=loadedVersion;
  const appUrl=new URL('../../app.js',import.meta.url);appUrl.searchParams.set('v',loadedVersion);
  try{await import(appUrl.href)}catch(error){
    const toast=document.querySelector('#toast');
    if(toast){toast.textContent='网页加载失败，请检查网络并刷新页面。';toast.className='show error'}
    console.error('Application failed to load',error);
  }
  window.addEventListener('focus',checkForUpdate);
  window.addEventListener('online',checkForUpdate);
  document.addEventListener('visibilitychange',checkForUpdate);
  setInterval(checkForUpdate,60000);
}
start();
