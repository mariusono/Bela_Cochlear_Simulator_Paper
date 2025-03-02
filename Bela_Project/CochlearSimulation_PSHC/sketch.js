let buttons = [];
let noButtons = 8;
let baseColor;
let signalColor;
let selectedColor;
let frameRateValue = 5;
let frameRateSlider;
let amplitudeSlider;

let frameCounter = 0;

let noChannels = 8; // here communicate with the bela.. 

//let buffer_inpGain = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]; // for inputGain
//let buffer_outGain = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]; // for outputGain

let buffer_inpGain = [1, 1, 1, 1, 1, 1, 1, 1]; // for inputGain
//let buffer_outGain = [1, 1, 1, 1, 1, 1, 1, 1]; // for outputGain

//let buffer_inpGain = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]; // for inputGain

//let buffer_inpGain = [1, 1, 1, 1, 1, 1]; // for inputGain
//let buffer_outGain = [1, 1, 1, 1, 1, 1]; // for outputGain


let amplitude = -30;

let sizeSliders = [];
//let buffer = [0,0,0];


//function preload() {
//  // Load the background image
//  backgroundImage = loadImage('dance-1.jpeg'); // Replace 'your-image-file.jpg' with your image file's name
//}

function setup() {
    //createCanvas(900,300);
    createCanvas(windowWidth, windowHeight);

    buffer_inpGain = Array(noChannels).fill(1.0);

    //createCanvas(backgroundImage.width, backgroundImage.height);
    frameRate(frameRateValue);

    baseColor = color(150);
    signalColor = color(0, 255, 0);
    selectedColor = color(0, 0, 255);

    amplitudeSlider = createSlider(-40, 12, -30, 0.1); // Slider range: -40-12, starting value: -30, step: 1
    amplitudeSlider.position(20, 220); // Position the slider on the canvas

	amplitudeLabel = createP('Global Volume');
    let labelX_amp_Slider = amplitudeSlider.x;
    let labelY_amp_Slider = amplitudeSlider.y - amplitudeSlider.height * 2;
    amplitudeLabel.position(labelX_amp_Slider, labelY_amp_Slider);


    frameRateSlider = createSlider(1, 60, frameRateValue, 1); // Slider range: 1-60, starting value: frameRateValue, step: 1
    frameRateSlider.position(20, 260); // Position the slider on the canvas

    frameRateLabel = createP('Frame Rate'); // Create a paragraph element for the label
    // frameRateLabel.parent('frameRateSlider'); // Set the slider as the parent of the label

    // // Adjust label position based on slider width
    // Calculate label position based on slider position
    let labelX = frameRateSlider.x;
    let labelY = frameRateSlider.y - frameRateSlider.height * 2;
    frameRateLabel.position(labelX, labelY);

    inputGain_slider = createSlider(0, 4, 1, 0.01); // Slider range: 10-100, starting value: 40, step: 1
    inputGain_slider.position(20, 300); // Position the slider on the canvas

    inputGain_label = createP('Input Gain'); // Create a paragraph element for the label
    let labelX_inpGain_size = inputGain_slider.x;
    let labelY_inpGain_size = inputGain_slider.y - inputGain_slider.height * 2;
    inputGain_label.position(labelX_inpGain_size, labelY_inpGain_size);

    // Position the sliders below the existing sliders
    let sliderX = 400;
    let sliderY = 200;

    for (let i = 0; i < noButtons; i++) {
        let button = new Button(i * 50 + 50, 100, 40, baseColor, i);
        buttons.push(button);
    }

    for (let i = 0; i < noButtons; i++) {
        let slider = createSlider(0, 4, 1, 0.01);
        slider.position(sliderX, sliderY); // Adjust the position as needed
        sizeSliders.push(slider);

        if ((i + 1) % 2 === 0) {
            sliderY += 20; // Move to the next row
        }
        else {
            sliderY += 20; // Move to the next row
        }
    }

	
	inputGainColumn_Label = createP('Input Gain Per Channel');

	inputGainColumn_Label.position(400, 160);
}

function draw() {
    background(220);

    let counter = Bela.data.buffers[0];

    // Update frame rate based on slider value
    frameRateValue = frameRateSlider.value();
    frameRate(frameRateValue);

    let bufferActiveElements = Bela.data.buffers[0];

    let randomIndices = [];
    while (randomIndices.length < 3) {
        let randomIndex = floor(random(buttons.length));
        if (!randomIndices.includes(randomIndex)) {
            randomIndices.push(randomIndex);
        }
    }

    for (let i = 0; i < buttons.length; i++) {
        //let isSignal = randomIndices.includes(i);
        let isSignal = bufferActiveElements[i];
        buttons[i].update(isSignal);
        //buffer_inpGain[i] = 1.3;
        //buffer_outGain[i] = 1.1;
        if (buttons[i].selected === true) {
            buffer_inpGain[i] = inputGain_slider.value();
            buttons[i].updateSliders(inputGain_slider.value());
        }
        buttons[i].display();
    }

	amplitude = amplitudeSlider.value();

    //Bela.data.sendBuffer(0, 'float', buffer);

    Bela.data.sendBuffer(0, 'float', buffer_inpGain);
    Bela.data.sendBuffer(1, 'float', amplitude);

    frameCounter++;
}

function mouseClicked() { // this is where the problem occurs 
    for (let button of buttons) {
        if (button.contains(mouseX, mouseY) && !button.selected) {
            for (let buttonAll of buttons) {
                buttonAll.deselect();
            }
            button.onSelect();
        }
        else if (button.contains(mouseX, mouseY) && button.selected) {
            button.deselect();
        }
        // else {
        //   button.deselect();
        // }
    }
    // for (let button of buttons) {
    //   if (button.contains(mouseX, mouseY)) {
    //     button.toggleSelect();
    //   } 
    //   else {
    //     button.deselect();
    //   }
    // }
}

class Button {
    constructor(x, y, size, baseColor, index) {
        this.x = x;
        this.y = y;
        this.size = size;
        this.baseColor = baseColor;
        this.active = false;
        this.selected = false;
        this.index = index;
        this.shapeType = floor(random(3)); // Choose a random shape type
    }

    contains(x, y) {
        return (
            x > this.x - this.size / 2 &&
            x < this.x + this.size / 2 &&
            y > this.y - this.size / 2 &&
            y < this.y + this.size / 2
        );
    }

    updateSize() {
        //this.size = sizeSlider.value();
        //this.size = Bela.data.buffers[0] * 50;
    }

    updateSliders(inGain) {
        sizeSliders[this.index].value(inGain);
        console.log(this.index);
    }

    onSelect() {
        this.selected = true;
    }

    toggleSelect() {
        this.selected = !this.selected;
    }

    update(isSignal) {
        if (this.selected) {
            this.active = false;
        } else {
            this.active = isSignal;
        }
    }

    deselect() {
        this.selected = false;
    }

    display() {
        noStroke();
        if (this.selected) {
            fill(selectedColor);
        } else if (this.active) {
            fill(signalColor);
        } else {
            fill(this.baseColor);
        }
        // ellipse(this.x, this.y, this.size);
        // Draw different shapes based on shapeType
        switch (this.shapeType) {
            case 0:
                rect(this.x - this.size / 2, this.y - this.size / 2, this.size, this.size);
                break;
            case 1:
                ellipse(this.x, this.y, this.size, this.size);
                break;
            case 2:
                triangle(this.x, this.y - this.size / 2, this.x - this.size / 2, this.y + this.size / 2, this.x + this.size / 2, this.y + this.size / 2);
                break;
        }
    }
}
