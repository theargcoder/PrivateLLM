#include <string>

std::string HTML_first =
    R"delim(
<!doctype html>
<html lang="en">
   <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      <title>Collapsible Think Div Example</title>

      <!-- Load MathJax for proper math rendering -->
      <script type="text/javascript">
         window.MathJax = {
            tex: {
               inlineMath: [['\\(', '\\)']],
               displayMath: [['\\[', '\\]']],
            },
            svg: {
               fontCache: 'global',
            },
         };
      </script>
      <script
         type="text/javascript"
         id="MathJax-script"
         async
         src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"
      ></script>

      <style>
         body {
            padding: 40px;
            line-height: 1.75;
            -webkit-font-smoothing: antialiased;
            background-color: rgb(50, 50, 50);
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            font-family: 'Courier New';
            font-size: 16px;
         }

         h1,
         h3 {
            color: rgb(200, 200, 200);
            margin-bottom: 16px;
         }

         p {
            font-size: 15.5px;
            color: rgb(255, 255, 255);
            margin-bottom: 5px;
         }

         /* Collapsible .think div styling */
         .think {
            background-color: rgb(70, 70, 70);
            color: rgb(255, 255, 255);
            padding: 20px;
            /* reduced padding for smoother transition */
            margin: 10px 0;
            border-left: 3px solid rgba(255, 175, 0, 0.45);
            border-radius: 15px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            font-family: 'Courier New';
            overflow: hidden;
            max-height: 100px;
            overflow-y: auto;
            /* Show only a portion by default */
            transition: max-height 0.5s ease-out;

            /* Enables vertical scrolling */
         }

         /* Expanded state shows full content (adjust max-height if needed) */
         .think.expanded {
            max-height: 700px;
         }

         .label-button-div {
            display: flex;
         }

         .button-div {
            display: block;
            align-content: end;
         }

         .toggle-label {
            background-color: rgba(255, 175, 0, 0.8);
            border: none;
            padding: 7px 12px;
            font-size: 15px;
            border-radius: 15px;
            font-family: 'Courier New', monospace;
            font-weight: 900;
            color: rgb(35, 35, 35);
         }

         /* Styling for the toggle button */
         .toggle-button {
            background-color: rgba(255, 175, 0, 0.9);
            border: none;
            opacity: 85%;
            padding: 1px 2px;
            font-size: 20px;
            cursor: pointer;
            border-radius: 5px;
            font-weight: 900;
            color: rgb(35, 35, 35);
            margin-left: 10px;
         }
      </style>
   </head>

   <body>
      <!-- Toggle button to expand/collapse the .think div -->
      <div class="label-button-div">
         <label class="toggle-label"> think </label>
         <div class="button-div">
            <button
               class="toggle-button"
               id="toggle-button"
               onclick="toggleThink()"
            >
               ↓
            </button>
         </div>
      </div>
   </body>
</html>
)delim";

std::string HTML_last =
    R"delim(
</body>
<script>
        // JavaScript function to toggle the expanded state
        function toggleThink() {
            const thinkDiv = document.getElementById('thinking');
            const toggleButton = document.getElementById('toggle-button');

            // Toggle the "expanded" class
            thinkDiv.classList.toggle('expanded');

            // Update the button label based on whether the div is expanded or not
            if (thinkDiv.classList.contains('expanded')) {
                toggleButton.textContent = '\u2191'; // Upwards Arrow (↑)
            } else {
                toggleButton.textContent = '\u2193'; // Downwards Arrow (↓)
            }
        }
    </script>
</html>
)delim";
