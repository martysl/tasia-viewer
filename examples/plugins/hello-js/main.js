// Hello JavaScript - example plugin
var version = tasia.viewer.getVersion();
tasia.viewer.notify("Hello from JavaScript! Version: " + version.version);

tasia.events.subscribe("chat.received", function(data) {
    if (data.message.toLowerCase().indexOf("hello") !== -1) {
        tasia.chat.send("Hi from JS plugin!");
    }
});
