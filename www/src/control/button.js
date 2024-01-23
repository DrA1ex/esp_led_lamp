import {Control} from "./base.js";

/**
 * @class
 * @extends Control<ButtonControl>
 */
export class ButtonControl extends Control {
    constructor(element) {
        super(element);

        this.addClass("button");
    }

    void
    setLabel(label) {
        this.element.innerText = label;
    }

    setOnClick(fn) {
        this.element.onclick = fn;
    }
}