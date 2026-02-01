// CDP Test Extension - Background Service Worker
// Tracks page loads and can communicate incognito status

chrome.runtime.onInstalled.addListener(() => {
    console.log('[CDP Test Extension] Installed');
});

// Listen for messages from content scripts
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.type === 'getIncognitoStatus') {
        // Check if the tab is in incognito mode
        if (sender.tab) {
            sendResponse({ incognito: sender.tab.incognito });
        } else {
            sendResponse({ incognito: false });
        }
    }
    return true;
});

// Log when extension starts
console.log('[CDP Test Extension] Background service worker started');
