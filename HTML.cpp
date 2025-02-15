#include <string>
std::string FULL_DOC =
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

         .content {
            font-size: 15.5px;
            color: rgb(255, 255, 255);
            margin-bottom: 5px;
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
      <script>
         // Function to clean up the math block patterns
         function cleanMathBlocks(element) {
            // Regex pattern to match:
            //   <p>\[</p>
            //   <p> ... </p>
            //   <p>\]</p>
            const pattern = /<p>\\\[<\/p>\s*<p>(.*?)<\/p>\s*<p>\\\]<\/p>/gs;
            const newHTML = element.innerHTML.replace(pattern, '\\[$1\\]');

            // Only update if a change was made to avoid unnecessary DOM updates.
            if (newHTML !== element.innerHTML) {
               element.innerHTML = newHTML;
            }
         }

         function cleanMathBlocksAfterDone(element) {
            // Ensure core math elements are enclosed in \[ ... \]
            const paragraphs = element.querySelectorAll('p');
            for (let i = 0; i < paragraphs.length; i++) {
               let content = paragraphs[i].innerHTML.trim();

               // Check if the paragraph contains a core math expression (e.g., \frac, \sum, etc.)
               const isCoreMath =
                  /\\(frac|sum|int|sqrt|prod|lim|log|ln|sin|cos|tan)\{/.test(
                     content
                  );

               if (isCoreMath) {
                  let prev = paragraphs[i - 1]?.innerHTML.trim();
                  let next = paragraphs[i + 1]?.innerHTML.trim();
                  if (!next) {
                     paragraphs[i].innerHTML += '<p>\\]</p>';
                  }

                  // Ensure opening delimiter \[
                  if (prev !== '\\[') {
                     paragraphs[i].innerHTML =
                        '<p>\\[</p>' + paragraphs[i].outerHTML;
                  }

                  // Ensure closing delimiter \]
                  if (next !== '\\]') {
                     paragraphs[i].outerHTML += '<p>\\]</p>';
                  }
               }
            }

            // Merge properly formatted math blocks
            const pattern = /<p>\\\[<\/p>\s*<p>(.*?)<\/p>\s*<p>\\\]<\/p>/gs;
            const newHTML = element.innerHTML.replace(pattern, '\\[$1\\]');

            if (newHTML !== element.innerHTML) {
               element.innerHTML = newHTML;
            }
         }
         /*
         function cleanMathBlocksAfterDone(element) {
            // Convert NodeList to an array to avoid live collection issues
            const paragraphs = Array.from(element.querySelectorAll('p'));

            // We'll loop over a static list of math paragraphs.
            paragraphs.forEach((p, index) => {
               const content = p.innerHTML.trim();
               // Check if this paragraph contains a core math expression
               const isCoreMath =
                  /\\(frac|sum|int|sqrt|prod|lim|log|ln|sin|cos|tan)\{/.test(
                     content
                  );

               if (isCoreMath) {
                  // --- Check for the opening delimiter ---
                  let prev = p.previousElementSibling;
                  if (
                     !prev ||
                     prev.tagName.toLowerCase() !== 'p' ||
                     prev.innerHTML.trim() !== '\\['
                  ) {
                     // Create a new paragraph with the opening delimiter
                     const openP = document.createElement('p');
                     openP.innerHTML = '\\[';
                     // Insert before the current paragraph
                     p.parentNode.insertBefore(openP, p);
                  }

                  // --- Check for the closing delimiter ---
                  let next = p.nextElementSibling;
                  if (
                     !next ||
                     next.tagName.toLowerCase() !== 'p' ||
                     next.innerHTML.trim() !== '\\]'
                  ) {
                     // Create a new paragraph with the closing delimiter
                     const closeP = document.createElement('p');
                     closeP.innerHTML = '\\]';
                     // Insert after the current paragraph
                     if (p.nextSibling) {
                        p.parentNode.insertBefore(closeP, p.nextSibling);
                     } else {
                        p.parentNode.appendChild(closeP);
                     }
                  }
               }
            });
         }
         */

         // Start observing once the document is fully loaded
         document.addEventListener('DOMContentLoaded', observeContentChanges);
      </script>
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
      <div class="content" id="content">
         <div class="think" id="thinking"></div>
      </div>

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
   </body>
</html>
)delim";
