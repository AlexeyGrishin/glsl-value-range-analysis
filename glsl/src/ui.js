import Quill from "quill"

hljs.configure({   
    languages: ['glsl']
});

class WarnHighlight
{
    constructor(quill) {
        this.el = document.createElement("div");
        //console.log(quill.root);
        quill.root.parentNode.appendChild(this.el);
    }

    show(rect, text) {
        this.el.style.position = "absolute";
        this.el.style.display = "block";
        this.el.className = "highlight-warning";
        this.el.style.left = rect.left + "px";
        this.el.style.top = rect.top + "px";
        this.el.style.width = rect.width + "px";
        this.el.style.height = rect.height + "px";
        this.el.title = text;
    }

    hide() {
        this.el.style.display = "none";
    }
}

const UIController = {
    init(src, parseSrcIntoReport) {
        this.checkButton = document.querySelector("#check");
        this.status = document.querySelector(".result h5");
        this.result = document.querySelector(".result pre");
        this.input = document.querySelector(".enter textarea");
        this.input.value = src;
        this.input.style.display = "none";
        this.checkButton.innerHTML = "Please wait...";
        this.checkButton.disabled = true;

        this.parseSrcIntoReport = parseSrcIntoReport;
        this.quill = new Quill(document.querySelector(".editor"), {
            useBR: false,
            modules: {
              syntax: true,              
              toolbar: '#toolbar'
            },
            theme: 'snow'
          });
        this.quill.setText(src);
        this.quill.formatText(0, this.quill.getLength(), "code-block", true);
        this.warnHighlights = [];
        this.quill.on('text-change', ()=>this.hideWarnings());
    },

    enableButton() {
        this.checkButton.innerHTML = "Check";
        this.checkButton.disabled = false;
        this.checkButton.addEventListener("click", () => this.process());
    },

    hideWarnings() {
        for (let w of this.warnHighlights) {
            w.hide();
        }
    },

    showWarnings(warnings) {
        this.hideWarnings();
        for (let i = 0; i < warnings.length; i++) {
            let w = warnings[i];
            let wh = this.warnHighlights[i];
            if (!wh) {
                wh = new WarnHighlight(this.quill);
                this.warnHighlights[i] = wh;
            }
            let lines = this.quill.getText().split("\n");
            let start = 0, li;
            for (li = 0; li < w.line; li++) {
                if (li === w.line-1) break;
                start += lines[li].length + 1;
            }
            let length = lines[li].length;
            wh.show(this.quill.getBounds(start, length), w.text);
        }
    },

    process() {
        /*let start = this.quill.getText().split("gl_FragColor.r")[0].length, length = "gl_FragColor.r".length;
        let rect = this.quill.getBounds(start, length);
*/
        this.result.innerHTML = "";
        this.status.innerHTML = "Processing...";
        try {
            let report = this.parseSrcIntoReport(this.quill.getText());
            if (report.warnings.length) {
                for (let w of report.warnings) {
                    let condition = '';
                    if (w.condition) {
                        condition = '<em>' + w.condition + '</em>';
                    }
                    this.result.innerHTML += `<p class='warning'><q>Line#${w.line}</q>${condition}${w.text}</p>`
                }
            } else {
                this.result.innerHTML += `<p class='success'>No Warnings!</p>`
            }
            this.status.innerHTML = "Done!";
            this.showWarnings(report.warnings);
        }
        catch (e) {
            console.error(e);
            this.status.innerHTML = "Error :(";
            this.result.innerHTML = JSON.stringify(e);
        }
    }
};

export default UIController;