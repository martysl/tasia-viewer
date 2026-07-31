# Hello Python - example Python plugin
import tasia

version = tasia.viewer.get_version()
tasia.viewer.notify("Hello from Python! Viewer version: " + version["version"])

@tasia.events.on("chat.received")
def on_chat(data):
    if "hello" in data["message"].lower():
        tasia.chat.send("Hi from Python plugin!")
