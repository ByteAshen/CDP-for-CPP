// CDP Test Extension - Content Script
// Injects a visible banner into every page to prove the extension is loaded

(function() {
    // Don't inject into chrome:// pages or extension pages
    if (window.location.protocol === 'chrome:' ||
        window.location.protocol === 'chrome-extension:') {
        return;
    }

    // Create the banner element
    const banner = document.createElement('div');
    banner.id = 'cdp-test-extension-banner';

    // Detect if we're in incognito mode
    // In MV3, we can't directly detect incognito from content scripts,
    // but we can set a marker that the test can check
    const isIncognito = chrome.extension.inIncognitoContext || false;

    banner.innerHTML = `
        <span class="cdp-ext-icon">&#9889;</span>
        <span class="cdp-ext-text">CDP Test Extension Active</span>
        <span class="cdp-ext-mode" id="cdp-ext-mode">${isIncognito ? '[INCOGNITO]' : '[NORMAL]'}</span>
        <span class="cdp-ext-url">${window.location.hostname}</span>
    `;

    // Set data attributes on the banner for easy detection via CDP
    banner.dataset.extensionLoaded = 'true';
    banner.dataset.incognito = isIncognito ? 'true' : 'false';
    banner.dataset.version = '1.0.0';

    // Insert at the top of the body
    if (document.body) {
        document.body.insertBefore(banner, document.body.firstChild);
    } else {
        // If body doesn't exist yet, wait for it
        document.addEventListener('DOMContentLoaded', function() {
            document.body.insertBefore(banner, document.body.firstChild);
        });
    }

    // Log to console for debugging
    console.log('[CDP Test Extension] Loaded on:', window.location.href);
    console.log('[CDP Test Extension] Incognito:', isIncognito);
})();
